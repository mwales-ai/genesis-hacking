Tasks for Claude:

# Incomplete Tasks

## Sprite Editor

### Sprite Viewer
* [X] Left panel needs an independent zoom control that controls how big the sprites are on the left side
* [X] Drag-to-reorder sprites in left panel
    * [X] Left-click and drag a sprite to a spot between other sprites to change display/storage order
    * [X] While moving a sprite group, show a cursor (vertical line) between the 2 sprite groups indicating drop position
* [X] Right panel canvas should grow to fit the entire sprite (or show scroll bars if zoom is too high)
* [ ] Double-click sprite in right panel jumps to Raw Tile Browser tab with matching palette, W, H, and address

### Sprite Collections
* [ ] Change tab name to "Sprite Animations"
* [ ] Add a button to add a sprite recording to the Collection combo
* [ ] Don't show stored sprite groups (from Sprite Viewer) in the combo box
* [ ] Combo box should switch between loaded sprite recording files

### Sprite Editor Tools
* [ ] Add a tool selection set of buttons
    * [ ] Current tool: pencil (touches one pixel at a time)
    * [ ] Bucket fill tool
    * [ ] Eye dropper tool (changes current color to color of pixel clicked on)
    * [ ] Brush tool with adjustable size

### Screen Captures
* [ ] Button to load a screen capture from file
* [ ] Button to add displayed screen capture to the game definition file
* [ ] Button to remove a screen capture from the game definition file (with confirmation dialog)
* [ ] Button to allow editing screen capture pattern tiles
* [ ] All the same sprite editor tools available on this screen
* [ ] In Edit mode, hovering over a pattern tile highlights all tiles sharing that tile in memory
* [ ] Drawing to a shared tile updates all instances of that shared tile

## BN-Genesis Plugin (Future — do not work on yet)

* [ ] Load a game definition file when a ROM is loaded and label all sprites at their ROM addresses
* [ ] Give each sprite a proper structure at its memory location
* [ ] Label each palette in the ROM with an appropriate structure
* [ ] Show a sprite in a custom Binja pane with palette selection

# Completed Tasks

(none yet)
