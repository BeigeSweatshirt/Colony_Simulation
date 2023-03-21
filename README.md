# Colony_Simulation
A colony simulation game where two colonies (R and B) try to dominate a map.

## Rules
- Each colony starts with a set number of spaces they control, as specified by a user at runtime. Controlled spaces are considered permanent, and will not change over the course of the sim. They are denoted on the outputted maps as 'R' and 'B'
- On each colonies turn, a missile is fired at a random location on the map. If it hits a controlled space, or a space occupied by the same team, nothing happens. If it hits an unoccupied space, or one that is occupied by the other team, the space is considered occupied, denoted by an 'r' or 'b.' After which, if the player controls and/or occupies the majority of the 8 spaces surrounding the newly occupied space, they occupy those spaces as well.
- Each space can only be occupied/controlled by one team.
- The simulation continues untill all spaces are controlled or occupied.

![demo](https://github.com/BeigeSweatshirt/Colony_Simulation/blob/main/demo.png?raw=true)

## How to Build
git clone https://github.com/BeigeSweatshirt/Colony_Simulation

Compile:
cd Colony_Simulation
gcc -o colonies colonies.c

## How to Run:
./colonies R B X Y

Where
- R and B are the number of spaces A and B control, respectively
- X and Y are the dimensions of the map
