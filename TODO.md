
Tasks for Claude:

# Sprite Editor To Do List

Going to sort the items out by the tab that they are for

* Sprite Viewer
  * Left panel needs an independent zoom control that controls how big the sprites are on the left side
  * Want to be able to left-click, and while holding mouse drag a sprite to a spot between other sprites and change the order they are displayed / stored in the file
  * While moving a sprite group, have a cursor (vertical line) appear between the 2 sprite groups that show where it will drop into place if let go of the mouse button
  * The right panel should have the canvas grow to fit the entire spirte (or scroll bars if zoom so high it doesn't fit fully on right panel
  * If a user double clicks on a sprite in the right panel, change the tab to the Raw Tile browser tab.  Change the palette, W and H settings to that of the sprite, 
    and jump to the address of that sprite in memory.
* Sprite Collections
  * Change tab name to sprite animations.
  * Have a button to add a sprite recording to the Collection combo.
  * Don't have stored sprite groups (that are now in the Sprite Viewer screen) in the combo box.
  * Combo box should switch between loaded sprite recording files
* Sprite Editor
  * Add a tool selection set of buttons
    * Our current tool will be pencil (touches one pixel at a time)
    * Add a bucket fill tool
    * Add an eye dropper tool (changes to current color to the color of the pixel clicked on)
    * Brush tool that has adjustable size
* Screen captures
  * Should have a button to load a screen capture from file
  * Should have a button to add the displayed screen capture to the game definition file
  * Should have a button to remove a screen capture from teh game definition file (should ask to confirm before doing it)
  * Should have a button to allow us to edit the screen capture pattern tiles
  * Should have all the same sprite editor tools on this screen as well
  * In Edit mode, as the mouse cursor hovers over a pattern tile, all the tiles that share that tile in memory should be highlighted as well
  * Drawing to a shared tile should update all of the shared tile

# BN-Genesis Plugin

Don't work on these tasks yet, these are future tasks

* Have a way to load a game definition file when a ROM is loaded, and label all the sprites at their proper ROM address
* Give each sprite in the ROM a proper structure for it's memory location of the sprite data
* Label each palette in the ROM and assign an appropriate structure
* Have a way to show a sprite in a custom Binja pane for viewing sprites.  This panel would also need a way to change the palette it is referencing
