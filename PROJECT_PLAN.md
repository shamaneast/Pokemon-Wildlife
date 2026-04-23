## **Goal (as finalized)**

Build a grass-only system where up to 5 wild Pokémon are visible in the overworld, wander around, can be battled by collision, despawn via hybrid logic, and use the same encounter species ratios as normal grass encounters. While this system is active, normal random grass encounters are disabled.

---

## **Why this fits your codebase cleanly**

Your project already has all the key primitives needed:

- Existing weighted wild encounter logic and map/time encounter access in **`wild_encounter.c`**.
- Tile classification for “this is a land encounter tile (grass)” in metatile behavior code.
- Overworld Pokémon object-event graphics IDs with species/shiny/female bits already defined.
- Follower-style Pokémon rendering path with dynamic palette loading including shiny handling and fallback behavior.
- Built-in wander movement types and config to animate wandering OW mons naturally.

So this is mostly a **system integration problem**, not a “build all infrastructure from scratch” problem.

---

## **Architecture Plan**

## **1) New subsystem: `visible_wilds`**

Create a dedicated manager (new C/H pair) that owns all runtime visible spawn state.

### **Proposed files**

- **`include/visible_wilds.h`**
- **`src/visible_wilds.c`**

### **Core state**

Use a fixed-size array of slots (size 5):

- **`active`** flag
- **`species`**
- **`level`**
- **`isShiny`**
- **`isFemale`** (optional for phase 1, but useful)
- **`mapX`**, **`mapY`**
- **`objEventId`** (or localId)
- **`remainingLifetimeSteps`** (hybrid despawn component)
- **`areaType`** (land only in phase 1)
- optional **`personalitySeed`** (for deterministic shiny + later extensions)

---

## **2) Config and tuning constants**

Add config values so balancing is easy without rewriting logic.

Examples:

- **`VISIBLE_WILD_MAX_ACTIVE = 5`**
- **`VISIBLE_WILD_SPAWN_CHANCE_PER_STEP = X%`**
- **`VISIBLE_WILD_DESPAWN_CHANCE_PER_STEP = Y%`** (per active mon)
- **`VISIBLE_WILD_LIFETIME_MIN/MAX`**
- **`VISIBLE_WILD_SPAWN_RADIUS`**
- **`VISIBLE_WILD_MIN_DISTANCE_FROM_PLAYER`**
- **`VISIBLE_WILD_ENABLE`**

Use these as compile-time constants initially (can later move to vars/debug tuning).

---

## **3) Species/level generation (ratio-accurate)**

This is the most important part: **use your existing encounter table selection flow** so visible species match random encounter ratios.

### **Rule**

For current map/time and land area, select from the same encounter source used by regular grass encounter flow in **`StandardWildEncounter`** path (where landMonsInfo is referenced and passed into generation logic).

### **Implementation style**

Two good options:

1. **Refactor** existing selection internals into reusable helper(s) callable by both random and visible systems.
2. Reproduce a small adapter wrapper around existing “choose index”/“create wild mon” style APIs while preserving table weights.

Option 1 is cleaner long-term.

---

## **4) Step hook integration**

Visible wild manager should update once per player step.

### **On each step**

1. If feature disabled or no valid grass context, do nothing.
2. Despawn pass over active slots (hybrid):
    - decrement lifetime;
    - if lifetime expired -> despawn;
    - else roll per-step despawn chance.
3. Spawn pass:
    - if active count < max, roll spawn chance;
    - if success, attempt to create one new spawn at a valid grass tile nearby.

This should be called from existing overworld/player movement step processing path (where random encounters are usually evaluated).

---

## **5) Spawn placement rules (grass only phase 1)**

Candidate tile must satisfy:

- land encounter tile check via **`MetatileBehavior_IsLandWildEncounter`** semantics.
- not blocked / not occupied by object / not player tile
- not out of map bounds
- within camera or chosen radius window
- avoid immediate adjacency spam if desired

If no valid tile found after N attempts, skip spawn this step.

---

## **6) Object event creation (follower-style look + wander)**

Create object events as OW mon graphics IDs:

- **`graphicsId = species + OBJ_EVENT_MON (+ shiny/female bits as needed)`** using existing bit system.
- Set movement type to wander (e.g. **`MOVEMENT_TYPE_WANDER_AROUND`**).
- This naturally leverages existing OW Pokémon graphics selection/palette loader path, including shiny palette behavior.

Because your engine already handles dynamic follower palettes and shiny flags, this avoids bespoke sprite code.

---

## **7) Collision -> battle transition**

When player collides with one of these visible spawn object events:

1. Identify which slot/object was touched.
2. Set battle input to that slot’s species/level/shiny (and optional gender/form).
3. Start a normal wild battle flow.
4. Remove or mark consumed the corresponding visible spawn.

This preserves standard battle UX while making encounter origin visible and diegetic.

---

## **8) Disable random grass encounters while active**

You requested full disable while visible mode is enabled.

Implement gating in standard random encounter path:

- if **`VISIBLE_WILD_ENABLE && current area is land grass`**, skip random check.
- leave surfing/fishing/etc untouched in phase 1.

The random path entry is **`StandardWildEncounter(...)`**, where land checks currently do normal random rolls and generation; this is where to guard for land mode suppression.

---

## **9) Shiny visible spawns**

Your engine supports shiny OW mons via object graphics ID bits and shiny palette flow already.

### **Practical behavior**

- Roll shiny at spawn creation (or derive from generated personality if you want strict battle parity).
- Mark graphics with shiny bit so shiny coloration appears in overworld.
- Ensure battle uses the same shiny result for that spawned mon.

The shiny-capable OW path is present in event object movement and constants as cited above.

---

## **10) Persistence and map transitions**

For phase 1 testing, simplest rule:

- Clear visible spawns on map change/warp.
- Rebuild naturally as player walks.

Later enhancement:

- Persist per-map active spawns in saveblock cache (optional).

---

## **11) Edge cases to handle explicitly**

- No encounter header for current map -> do not spawn.
- Encounter header exists but no land table -> do not spawn.
- Player indoors or on non-encounter tile -> don’t update/ spawn.
- Object event pool full -> skip spawn gracefully.
- Script/cutscene/freeze states -> pause manager updates.
- Repel behavior:
    - For phase 1, decide whether repel suppresses visible spawns or only battle trigger.
    - Recommend: if repel active and spawned level too low, either don’t spawn or despawn immediately to avoid confusion.

---

## **12) Rollout strategy**

### **Phase A (minimal viable)**

- Spawn/despawn manager
- Max 5
- Grass-only placement
- Wander movement
- Collision battle
- Disable random grass encounters

### **Phase B**

- Better spawn spacing + avoidance heuristics
- Better despawn visuals (poof effect)
- Debug overlay/commands

### **Phase C**

- Surf/water visible mons
- Time/weather behavior
- Rare forced shiny events etc.

---

## **13) Suggested initial balancing (quick test defaults)**

- Max active: 5
- Spawn chance per step: 12%
- Despawn chance per mon per step: 3%
- Lifetime range: 80–180 steps
- Spawn distance from player: 3–8 tiles

This creates a lively but not overcrowded field.

---

