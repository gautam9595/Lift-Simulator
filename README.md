# Lift-Simulator
A simulation of lifts depicting the Inter-Process Communication by shared memory and semaphores.

### Number of Persons
```
#define N 10
```
Each Person is a child process. Each person is initally at a random floor. After Randomly waiting for sometime depending upon an exponential distribution, person choose a destination floor and wait for the correct direction lift to stop at that floor.
### Number of Floors
```
#define F 6
```
Each floor has associated mutexes to synchronize person entering in a lift and leaving the lift at that floor.
### Number of Lift
```
#define M 3
```
Each Lift is a child process. Each lift is initally at a random floor going either upward or downward and it also has a capacity. Even if there's no person inside a lift, it never stops moving. Upon Reaching top or bottom Floor, it changes the direction and starts moving. If any lift is working at it's full capacity, it will not stop at a floor where a person is waiting to get picked up.

## Details Shown
The simulation is being shown to give a GUI feel
* It shows the number of persons at each floor waiting to get picked up.
* Each lift is associated with a status board showing whether it is moving or stopped at any floor
  * If moving, It shows between which floors it is moving and how many person are getting off on which floor.
  * If stopped at a floor, It shows how many persons are getting on and off the lift.
## Ending Program
For Safe termination of program, unlinking & release of shared memories, and release of semaphores, TSTP signal (i.e., CTRL+Z) is being overrriden to stop the program. 
