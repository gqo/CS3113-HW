# CS3113 - Homework 2
Description of the assignment was:
* Make PONG!
* Doesn't need to keep score but must detect player wins
* Can use images or untextured polygons
* Can use keyboard, mouse, or joystick input
## Running
The game can be run on a Linux system by navigating to the game/ directory and typing the command `make run`.

Alternatively, one can type `make` to generate a mygame binary and then type `./mygame`
## Detecting Wins:
The boundary walls of the arena change color based on which player one the previous round.

I.e. if the red paddle player scores on blue, the walls turn red and vice versa.
## Movement
The red paddle can be controlled using the W and S keys.

The blue paddle can be controlled using the Up and Down arrows.
## Extra features
The ball is intended to speed up as the round progresses making it harder and harder for players to correctly keep track of the ball.