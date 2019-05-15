# CS3113 - Homework 4
Description of the assignment was:
* Make a simple scrolling platformer game demo
* It must use a tilemap or static/dynamic entities
* Must have a controllable player character that interacts with at least one other dynamic entity (enemy or item)
* It must scroll to follow your player with the camera
* It must use FIXED TIMESTEP
## Running
The game can be run on a Linux system by navigating to the game/ directory and typing the command `make run`.

Alternatively, one can type `make` to generate a mygame binary and then type `./mygame`
## Movement
The player can be moved left and right using the LEFT and RIGHT arrow keys respectively.

The player can jump with the SPACE key.
## Winning or Losing
The player is reset back to starting position upon contact with an enemy. Additionally, the player sprite is changed to a redder hue.