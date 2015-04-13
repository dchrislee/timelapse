cycles
-----

BUTTON 1 pressed
-> wake up
--> launch camera
-> continue sleep

BUTTON 2 pressed
-> wake up
-> show prev digit
--> if button pressed once again
--> data++ (0 - disable timer)
-> blink 5 times
--> show digit for 5 sec
---> turn off digit
----> disable Shift Register (SR)
-> run WDT timer
-> sleep

states
----
SINGLE_CAMERA_SHOT
SHOW_DIGIT
BLINK_DIGIT
POWER_CUT
WDT_SETUP


0 - disabled
1 - 1 sec
2 - 2 sec
3 - 3 sec
4 - 4 sec
5 - 5 sec
6 - 6 sec
7 - 7 sec
8 - 8 sec
9 - 9 sec
A - 1 min
B - 2 min
C - 3 min
D - 4 min
E - 5 min
F - 6 min
G - 7 min
H - 8 min
