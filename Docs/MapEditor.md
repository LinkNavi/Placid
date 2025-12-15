# Placid Map Editor - Complete Guide

A comprehensive guide to creating levels with the Placid Map Editor.

## Table of Contents
- [Getting Started](#getting-started)
- [Interface Overview](#interface-overview)
- [Camera Controls](#camera-controls)
- [Creating Geometry](#creating-geometry)
- [Working with Entities](#working-with-entities)
- [Selection and Manipulation](#selection-and-manipulation)
- [Testing Your Map](#testing-your-map)
- [Properties and Settings](#properties-and-settings)
- [File Operations](#file-operations)
- [Tips and Tricks](#tips-and-tricks)
- [Common Workflows](#common-workflows)

---

## Getting Started

### Launching the Editor
```bash
./run.sh
# or
./placid --editor
```

### First Time Setup
When you first open the editor, you'll see:
- A **grid** on the ground (XZ plane)
- **Coordinate axes**: Red (X), Blue (Z), Green (Y)
- A default **floor** and **player spawn** (if creating new map)
- **UI panels** on the sides

---

## Interface Overview

### Main Window Areas

```
┌─────────────────────────────────────────────────────────┐
│ Menu Bar (File, Edit, View, Create, Tools)             │
├──────┬──────────────────────────────────────────┬──────┤
│      │                                          │      │
│ Tool │         3D Viewport                      │Props │
│ Bar  │         (Grid + Geometry)                │Panel │
│      │                                          │      │
│      │                                          │      │
├──────┴────────────────┬─────────────────────────┴──────┤
│ Brush List            │ Entity List                    │
│ - Floor               │ - PlayerSpawn                  │
│ - Wall1               │ - Light1                       │
└───────────────────────┴────────────────────────────────┘
│ Status Bar: Tool | Counts | Grid Size | Save Status   │
└─────────────────────────────────────────────────────────┘
```

### Panels

**Tool Bar (Left)**
- Quick access to tools: Select, Move, Rotate, Scale
- Brush creation: Box, Cylinder, Wedge
- Entity placement, Vertex editing

**Properties Panel (Right)**
- Map properties (name, author)
- Grid settings
- Selected brush/entity properties
- Type-specific properties

**Brush List (Bottom Left)**
- All brushes in the map
- Click to select
- Shows brush names/IDs

**Entity List (Bottom Right)**
- All entities in the map
- Click to select
- Shows entity types and names

**Status Bar (Bottom)**
- Current tool
- Object counts
- Grid size
- Save status

---

## Camera Controls

### Movement
| Key/Mouse | Action |
|-----------|--------|
| `W` | Move forward |
| `S` | Move backward |
| `A` | Move left |
| `D` | Move right |
| `Q` | Move down |
| `E` | Move up |

### View
| Key/Mouse | Action |
|-----------|--------|
| `Right Mouse` + Drag | Rotate camera around target |
| `Middle Mouse` + Drag | Pan camera (move target) |
| `Scroll Up` | Zoom in (decrease distance) |
| `Scroll Down` | Zoom out (increase distance) |

### Tips
- **Focus on area**: Use middle mouse to pan to the area you want to work on
- **Zoom in for detail**: Scroll to get closer for precise placement
- **Orbit for inspection**: Right mouse drag to view from all angles

---

## Creating Geometry

### Brush Types

#### Box (Cuboid)
The most common brush type for walls, floors, platforms.

**To create:**
1. Press `B` or click Box tool in toolbar
2. Click on the grid where you want one corner
3. Drag to define the size
4. Release to create

**Use cases:**
- Floors and ceilings
- Walls
- Platforms
- Pillars
- Doors

#### Cylinder
Round pillars, columns, or curved geometry.

**To create:**
1. Press `C` or click Cylinder tool
2. Click center point
3. Drag to define radius and height
4. Release to create

**Use cases:**
- Pillars
- Towers
- Round platforms
- Decorative elements

#### Wedge (Ramp)
Angled surfaces for slopes and ramps.

**To create:**
1. Select Wedge tool from menu: Create → Brushes → Wedge/Ramp
2. Click starting point (low end)
3. Drag to ending point (high end)
4. Release to create

**Use cases:**
- Ramps
- Stairs (multiple wedges)
- Angled walls
- Terrain slopes

### Brush Creation Workflow

```
Step 1: Select Tool
├─ Press hotkey (B, C) or
└─ Menu: Create → Brushes → Type

Step 2: Place on Grid
├─ Left click on grid
└─ Grid snapping (if enabled)

Step 3: Size it
├─ Drag to define dimensions
├─ Watch preview (blue wireframe)
└─ Height auto-set if needed

Step 4: Finish
├─ Release mouse
└─ Brush appears in scene

Step 5: Adjust (Optional)
├─ Properties panel
├─ Position, size, color
└─ Flags (solid, water, etc.)
```

### Grid Snapping

**Enable/Disable:** Properties Panel → Grid → "Snap to Grid"

**Grid Size:** Adjust in Properties Panel
- `0.25` - Fine detail
- `0.5` - Detailed work
- `1.0` - Standard (default)
- `2.0` - Large structures
- `4.0`+ - Rough layout

**Grid Height:** Vertical position of grid plane
- Change to work on different floors
- Useful for multi-level maps

---

## Working with Entities

### Entity Types

#### Spawn Points
**Player Start** - Single player spawn
- Menu: Create → Spawn Points → Player Start
- Shows as green box
- Place at start of level

**Deathmatch** - Multiplayer spawn
- Menu: Create → Spawn Points → Deathmatch
- Multiple spawns for respawning
- Spread around map

**Team Spawns** - Team-based modes
- Red Team: Create → Spawn Points → Team Red
- Blue Team: Create → Spawn Points → Team Blue
- Place in team bases

#### Lights
**Point Light** - Omnidirectional light
- Menu: Create → Lights → Point
- Properties: Color, Intensity, Radius
- Use for general illumination

**Spot Light** - Directional cone
- Menu: Create → Lights → Spot
- Properties: Color, Intensity, Radius, Direction
- Use for focused lighting

**Environment Light** - Ambient/skybox light
- Menu: Create → Lights → Environment
- Global illumination
- One per map

#### Items
**Health** - Health pickup
- Menu: Create → Items → Health
- Properties: Amount (25, 50, 100), Respawn Time

**Armor** - Armor pickup
- Menu: Create → Items → Armor
- Properties: Amount, Respawn Time

**Ammo** - Ammunition
- Menu: Create → Items → Ammo
- Properties: Amount, Respawn Time

#### Weapons
- Shotgun, Rocket, Railgun, Plasma
- Menu: Create → Weapons → [Type]
- Auto-respawn after pickup

#### Triggers
**Trigger Once** - Activates once when touched
- Menu: Create → Triggers → Once
- Use for cutscenes, one-time events

**Trigger Multiple** - Activates repeatedly
- Menu: Create → Triggers → Multiple
- Use for doors, elevators

**Hurt Trigger** - Damages player
- Menu: Create → Triggers → Hurt
- Properties: Damage per second
- Use for lava, hazards

**Push Trigger** - Applies force
- Menu: Create → Triggers → Push
- Properties: Force vector (X, Y, Z)
- Use for jump pads, wind

**Teleport** - Instant transport
- Menu: Create → Triggers → Teleport
- Properties: Destination entity
- Use for portals

#### Func Entities
**Door** - Sliding door
- Menu: Create → Func → Door
- Properties: Move distance, Speed
- Triggered by buttons or proximity

**Button** - Activates targets
- Menu: Create → Func → Button
- Properties: Target entity
- Use to trigger doors, events

**Platform** - Moving platform
- Menu: Create → Func → Platform
- Properties: Path, Speed
- Use for elevators, moving floors

**Rotating** - Spinning object
- Menu: Create → Func → Rotating
- Properties: Axis, Speed
- Use for fans, gears

### Entity Placement Workflow

```
Step 1: Choose Entity
└─ Menu: Create → Category → Type

Step 2: Click to Place
├─ Tool switches to "Place Entity"
└─ Click on grid/surface

Step 3: Adjust Properties
├─ Entity appears
├─ Select in Entity List
└─ Edit in Properties Panel

Step 4: Fine-tune Position
├─ Select entity
├─ Use Move tool (W) or
└─ Edit Position in Properties
```

---

## Selection and Manipulation

### Selecting Objects

**Select Single:**
- Click tool (`Q` or Select in toolbar)
- Left click on object
- Highlighted in yellow/orange

**Select from Lists:**
- Click in Brush List or Entity List
- Jumps to that object

**Select Multiple:** (Planned feature)
- Currently one at a time

**Deselect:**
- Press `Esc`
- Or click empty space

### Tools

#### Select Tool (`Q`)
- Default tool
- Click to select brushes or entities
- View properties

#### Move Tool (`W`)
*Currently in development*
- Will allow dragging objects
- For now, use Properties panel

#### Rotate Tool (`E`)
*Currently in development*
- Will allow rotating objects
- For now, use Properties panel

#### Scale Tool (`R`)
*Currently in development*
- Will allow resizing objects
- For now, use Properties panel

### Manipulating Selection

**Delete:**
- Select object
- Press `Delete` key
- Or Menu: Edit → Delete

**Duplicate:**
- Select object
- Press `Ctrl+D`
- Or Menu: Edit → Duplicate
- Creates copy offset slightly

**Move (Manual):**
1. Select object
2. Properties Panel → Position
3. Edit X, Y, Z values

**Rotate (Manual):**
1. Select object (entity only)
2. Properties Panel → Rotation
3. Edit angles in degrees

**Resize (Manual):**
1. Select brush
2. Properties Panel → (varies by type)
3. Edit dimensions

---

## Testing Your Map

### Play Mode

You can test your map at any time by pressing **F5** or clicking the "Test Map" button.

**To Enter Play Mode:**
1. Press `F5`
2. OR Menu: Tools → Test Map (future)

**What Happens:**
- Editor UI disappears
- You spawn at a Player Start entity
- Camera switches to first-person
- Mouse is captured for looking
- Physics and collision enabled

**Play Mode Controls:**
| Key | Action |
|-----|--------|
| `W` | Move forward |
| `S` | Move backward |
| `A` | Move left |
| `D` | Move right |
| `Space` | Jump |
| `Shift` | Sprint |
| `Mouse` | Look around |
| `Esc` | Return to editor |

**To Exit Play Mode:**
- Press `Esc`
- Returns to editor at last camera position
- All editor state preserved

### What Gets Tested

**Geometry:**
- ✅ Brush collision (walk on floors, hit walls)
- ✅ Platforms and height differences
- ✅ Ramps and slopes
- ✅ Physics and gravity

**Movement:**
- ✅ Walking and running
- ✅ Jumping
- ✅ Sprint speed
- ✅ Ground detection

**Spawn Points:**
- ✅ Player spawns at first Player Start entity
- ✅ Falls back to (0, 2, 0) if no spawn found

**Not Yet Tested:**
- ❌ Items (not yet functional)
- ❌ Weapons (not yet functional)
- ❌ Triggers (not yet functional)
- ❌ Enemies/NPCs (not yet implemented)
- ❌ Lighting (no dynamic lights yet)

### Testing Workflow

```
Step 1: Build Your Map
├─ Create geometry
├─ Place spawn point
└─ Save (Ctrl+S)

Step 2: Test It
├─ Press F5
├─ Walk around
├─ Check collision
└─ Test jumps/platforms

Step 3: Return to Editor
├─ Press Esc
└─ Make adjustments

Step 4: Iterate
└─ Repeat test cycle
```

### Common Test Scenarios

**Check Scale:**
```
Test: Can player walk through doorways?
├─ Door too narrow? → Widen
├─ Ceiling too low? → Raise
└─ Feels right? → Good!
```

**Check Platforms:**
```
Test: Can player reach platform?
├─ Too high? → Add ramp or lower
├─ Jump distance okay? → Good!
└─ Hard to land on? → Make wider
```

**Check Collisions:**
```
Test: Walk into all walls
├─ Fall through floor? → Check brush flags
├─ Stuck in geometry? → Fix overlaps
└─ Smooth movement? → Good!
```

**Check Spawns:**
```
Test: Player spawns correctly
├─ Spawn in wall? → Move spawn point
├─ Spawn in air? → Check Y position
└─ Good spawn location? → Good!
```

### Testing Tips

**Quick Iterations**
- Keep editor open
- F5 to test, Esc to edit
- No need to restart
- Changes take effect immediately

**Test Early, Test Often**
- Test after creating each room
- Don't wait until map is "done"
- Easier to fix issues early
- Prevents major redesigns

**Scale Testing**
- First thing to test
- Player is ~1.8 units tall
- Doorways should be 2.5-3 units
- Test jumping (player can jump ~2 units)

**Navigation Testing**
- Can player reach all areas?
- Are paths clear?
- Any dead ends unintentional?
- Can player return from areas?

**Collision Testing**
- Walk along all walls
- Try to get stuck in corners
- Jump on all platforms
- Test slopes and ramps

### Troubleshooting Play Mode

**Player spawns in the air and falls**
→ Add a Player Start entity at ground level

**Player falls through floor**
→ Check floor brush has BRUSH_SOLID flag enabled
→ Make sure floor vertices form a solid surface

**Can't move or look around**
→ Make sure you're in play mode (press F5)
→ Press Esc and re-enter if stuck

**Stuck in geometry**
→ Press Esc to return to editor
→ Check for overlapping brushes
→ Ensure walls are properly aligned

**Spawn point not working**
→ Check Entity List for Player Start
→ Make sure Y position is above floor
→ Try moving spawn to open area

**Camera feels wrong**
→ This is player camera (different from editor)
→ Mouse sensitivity will be adjustable later
→ Return to editor (Esc) for orbit camera

---

## Properties and Settings

### Map Properties

**Name:** Map display name
- Example: "Death Arena", "Industrial Complex"
- Used in loading screens

**Author:** Your name
- Credited in map info

### Grid Settings

**Grid Size:** Snap increment
- Smaller = more precise
- Larger = faster layout
- Common: 0.5, 1.0, 2.0

**Snap to Grid:** On/Off
- On: Objects snap to grid
- Off: Free placement

**Show Grid:** Visibility toggle
- Usually keep On
- Turn Off for screenshots

**Grid Height:** Vertical position
- Change to work on floors
- Example: 0 (ground), 4 (first floor), 8 (second floor)

**Grid Plane:** Which plane to work on
- XZ (Floor) - Default, horizontal
- XY (Front) - Vertical, front view
- YZ (Side) - Vertical, side view

### Brush Properties

**Name:** Custom name for brush
- Example: "MainFloor", "WestWall"
- Makes organization easier

**Color:** RGB color
- Visual identification
- Preview color (no textures yet)

**Texture ID:** Texture reference
- Currently unused
- Future: will map to texture

**Flags:** Brush behavior
- ☑ **Solid** - Blocks player
- ☐ **Detail** - Visual only, optimized
- ☐ **Trigger** - Trigger volume
- ☐ **Water** - Water volume (blue)
- ☐ **Lava** - Lava/damage (red)
- ☐ **Ladder** - Climbable surface
- ☐ **Clip** - Invisible blocker
- ☐ **No Collide** - No collision

### Entity Properties

**Name:** Custom entity name
- Example: "FrontDoorButton"
- For linking/targeting

**Position:** X, Y, Z coordinates
- Precise placement

**Rotation:** Pitch, Yaw, Roll
- Orientation in degrees

**Scale:** Size multiplier
- Usually 1.0
- Increase for larger

**Type-Specific:**

*Light:*
- Color (RGB)
- Intensity (0-100)
- Radius (range)

*Hurt Trigger:*
- Damage per second

*Push Trigger:*
- Force X, Y, Z

*Door:*
- Move distance X, Y, Z
- Speed

*Items:*
- Amount
- Respawn time (seconds)

---

## File Operations

### New Map
**Command:** `Ctrl+N` or File → New

Creates blank map with:
- Empty brushes list
- Empty entities list
- Default settings

⚠️ **Warning:** Unsaved changes will be lost

### Save Map
**Command:** `Ctrl+S` or File → Save

Saves to current file or `map.pcd` if new.

**File Format:** `.pcd` (Placid Content Data)
- Binary format
- Includes all brushes and entities
- Compact and fast to load

### Save As
**Command:** File → Save As

Choose custom filename and location.

### Open Map
**Command:** `Ctrl+O` or File → Open
*Currently in development*

Will load existing `.pcd` files.

### Export
**Command:** File → Export .pcd

Explicitly save as PCD format.

---

## Tips and Tricks

### General Tips

**Start with Layout**
1. Block out main areas with boxes
2. Use large grid size (2.0 or 4.0)
3. Get proportions right first
4. Detail later with smaller grid

**Layer Your Detail**
1. Base geometry (floor, walls, ceiling)
2. Architecture (pillars, platforms)
3. Detail brushes (trim, small features)
4. Entities last (spawns, items, lights)

**Use Naming**
- Name brushes: "Floor_Main", "Wall_North"
- Name entities: "RedBaseSpawn", "DoorButton01"
- Makes finding things easier

**Color Coding**
- Floors: Gray
- Walls: Brown
- Platforms: Dark gray
- Detail: Various colors
- Helps visual organization

### Performance Tips

**Use Detail Flag**
- Small decorative brushes: Flag as Detail
- Helps optimization
- Visual only, no collision impact

**Avoid Tiny Brushes**
- Keep brushes reasonable size
- Many tiny brushes = slower
- Combine where possible

**Limit Entities**
- Each entity has overhead
- Reuse when possible
- Delete unused entities

### Design Tips

**Scale Reference**
- Player is ~1.8 units tall
- Doors: ~2.5-3 units high
- Ceilings: 4-8 units high
- Corridors: 3-4 units wide

**Gameplay Flow**
- Multiple paths between areas
- Open areas for combat
- Tight areas for ambush
- Vertical variation (platforms)

**Item Placement**
- Health near combat areas
- Armor in risky spots
- Weapons in central locations
- Ammo throughout map

**Lighting**
- Light every room/area
- Vary intensity for atmosphere
- Use color for mood
- Avoid pure white (boring)

### Workflow Tips

**Save Often**
- `Ctrl+S` frequently
- Before major changes
- After completing sections

**Use Undo/Redo**
- `Ctrl+Z` to undo
- `Ctrl+Y` to redo
- Experiment freely

**Test Geometry**
- Create, test, iterate
- Don't aim for perfect first try
- Duplicate and modify

**Organize Lists**
- Keep brush/entity lists manageable
- Delete unused objects
- Name important objects

---

## Common Workflows

### Building a Room

```
1. Create Floor
   ├─ Tool: Box (B)
   ├─ Size: 20x1x20
   └─ Name: "MainRoomFloor"

2. Create Walls
   ├─ Tool: Box (B)
   ├─ Four walls: 20x5x1 each
   ├─ Position around floor
   └─ Name: "Wall_North", etc.

3. Create Ceiling
   ├─ Tool: Box (B)
   ├─ Size: 20x1x20
   ├─ Position: Y=5
   └─ Name: "MainRoomCeiling"

4. Add Player Spawn
   ├─ Create → Player Start
   ├─ Place in center
   └─ Y = 0.1 (on floor)

5. Add Light
   ├─ Create → Point Light
   ├─ Place in center
   ├─ Y = 4 (near ceiling)
   └─ Intensity: 1.0, Radius: 15

6. Save
   └─ Ctrl+S
```

### Creating a Platform

```
1. Create Platform Base
   ├─ Tool: Box (B)
   ├─ Size: 5x1x5
   ├─ Height: Y=3 (above ground)
   └─ Name: "Platform01"

2. Create Ramp
   ├─ Tool: Wedge
   ├─ Connect ground to platform
   ├─ Slope angle ~30 degrees
   └─ Name: "PlatformRamp"

3. Add Jump Pad (Optional)
   ├─ Create → Trigger → Push
   ├─ Place at base
   ├─ Force: 0, 20, 0 (upward)
   └─ Players can jump up

4. Add Item on Top
   ├─ Create → Items → Armor
   └─ Reward for reaching platform
```

### Setting Up Deathmatch Spawns

```
1. Distribute Spawns
   ├─ Create 8-16 spawn points
   ├─ Spread evenly around map
   └─ Avoid clustering

2. Check Spawn Safety
   ├─ Not facing walls
   ├─ Clear space around
   └─ Not in hazards

3. Balance Spawns
   ├─ Equal access to weapons
   ├─ Equal access to items
   └─ No "spawn of death"

4. Test in Game
   └─ (Future: play testing)
```

### Creating a Door

```
1. Create Door Frame
   ├─ Box: 3x4x0.5 (opening)
   └─ Two side walls

2. Create Door Brush
   ├─ Box: 3x4x0.2
   ├─ Flag: Detail
   └─ Color: Brown (door-like)

3. Add Door Entity
   ├─ Create → Func → Door
   ├─ Place on door brush
   └─ Move Distance: Y=4 (up)

4. Add Button
   ├─ Create → Func → Button
   ├─ Place nearby
   └─ Target: Door entity name

5. Test
   └─ Button triggers door
```

### Building Multi-Level

```
1. Ground Floor (Y=0)
   ├─ Floor at Y=-1 to 0
   ├─ Walls, rooms
   └─ Ceiling at Y=4

2. Second Floor (Y=4)
   ├─ Set Grid Height: 4
   ├─ Floor at Y=4 to 5
   ├─ Walls, rooms
   └─ Ceiling at Y=8

3. Connect Floors
   ├─ Stairs (multiple wedges)
   ├─ Elevator (moving platform)
   └─ Teleporter

4. Add Spawns per Floor
   └─ Players can spawn on any level
```

---

## Keyboard Shortcuts Reference

### File
- `Ctrl+N` - New map
- `Ctrl+O` - Open map (planned)
- `Ctrl+S` - Save map

### Edit
- `Ctrl+Z` - Undo
- `Ctrl+Y` - Redo
- `Delete` - Delete selected
- `Ctrl+D` - Duplicate selected
- `Ctrl+A` - Select all
- `Esc` - Deselect

### Tools
- `Q` - Select tool
- `W` - Move tool
- `E` - Rotate tool
- `R` - Scale tool
- `B` - Box brush tool
- `C` - Cylinder tool
- `V` - Vertex edit (planned)

### Camera
- `W/A/S/D` - Move camera
- `Q/E` - Move down/up
- `Right Mouse + Drag` - Rotate
- `Middle Mouse + Drag` - Pan
- `Scroll` - Zoom

### View
- `F1` - Toggle editor/game (planned)

---

## Troubleshooting

**Grid not visible**
→ Properties → Grid → Show Grid ☑

**Can't place objects**
→ Check you have a tool selected (B, C, or Create menu)

**Object too small/large**
→ Adjust Grid Size, or edit in Properties after creation

**Can't find object**
→ Check Brush/Entity lists, click to select and camera focuses

**Undo doesn't work**
→ Only works for specific operations (create, delete, modify)
→ Save frequently instead

**Changes not saving**
→ Press Ctrl+S, check for "Saved" in status bar

**Selection disappeared**
→ Pressed Esc or clicked empty space
→ Find in Brush/Entity list

---

## Next Steps

**Practice Projects:**
1. Simple box room with spawn and light
2. Room with platforms and ramps
3. Multi-room layout with doorways
4. Small arena with item pickups
5. Complete deathmatch map

**Learn More:**
- [README.md](README.md) - General documentation
- [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) - Technical details
- [QUICKSTART.md](QUICKSTART.md) - Installation guide

**Get Help:**
- Check this guide first
- Try Undo (Ctrl+Z) if stuck
- Start new map if needed (Ctrl+N)
- Save often to avoid losing work

---

**Happy Mapping! 🎮**
