# Ping Pong

## [Game Demo Link](https://youtu.be/0kyKRH9ca48)

### Description
A classic ping pong game implemented using concurrent state machines. The game can be played with one or two players. When there is one player, the other paddle is controlled by the computer. Players control the paddles with potentiometers, and the game is displayed on an LCD screen. A second LCD display shows the score and game mode. A remote control is used to start and reset the game, and to change the mode between one and two player mode. The players hit the ball back with their paddles. If the ball gets past their paddle, the other player gets a point. Whoever gets 3 points first wins the game. 

### User Guide
When there is no game ongoing, the user presses the right button to toggle between one player and 2 player mode. The mode is indicated by “1P” or “2P” on the bottom center of the text display. In two player mode, each potentiometer controls a paddle. The user presses the left button to start a game. The user can press the button during or after the game to return to the start screen.

Players twist one of the potentiometers to control their paddle. Players can see the score on the text display in the bottom corners. When someone wins the game by getting 3 points, a gold square appears on the winning player’s side of the screen, and a brown square appears on the losing player’s side of the screen. A message on the top row of the text display says who won the game.

### Hardware Components
- ATMega328 Microcontroller
- 3 potentiometers
- 2 buttons
- 1.44" Colorful SPI TFT LCD Display ST7735 128X128 Replace 5110/3310 LCD
- 1602 LCD display


