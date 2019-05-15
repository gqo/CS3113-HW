# CS3113 - Final Project
Description of the assignment was:

* Must have a title screen and a proper mode for game over, etc...
* Must have a way to quit the game
* Must have music and sound effects
* Must have at least 3 different levels or be procedurally generated
* Must be either local multiplayer or have AI (or both!)
* Must have at least some animation or particle effects

## Running
The game can be run on a Linux system by navigating to the game/ directory and typing the command `make run`.

Alternatively, one can type `make` to generate a mygame binary and then type `./mygame`

## Movement

* Both players possess the ability to double jump once in the air.

* Both players possess the ability to wall jump.

### Player 1
Player 1 is controlled with the following bindings:

    * Keypad8: Slow descent
    * Keypad5: Speed descent
    * Keypad4: Move left
    * Keypad6: Move right
    * R-CTRL:  Jump
    * R-Shift: Dash

### Player 2
Player 2 is controlled with the following bindings:

    * W:       Slow descent
    * S:       Speed descent
    * A:       Move left
    * D:       Move right
    * L-CTRL:  Jump
    * L-Shift: Dash

## Winning or Losing
A player will lose if they hit a spike or are forced off screen by an advancing player.

A player will win if they touch the goal zone.

## Quitting

One can quit the game at the main menu by pressing ESCAPE.

One can quit to the main menu at any other point in the game by pressing ESCAPE.