#### Fantastic Flamingos Project 2
###### Featuring:
Ben Hanson
Elise Macabou
Joe Cohen

This project was done by all three of us on one VM that we used for testing! We used pair programming and group discussion to develop and debug it together. At first we ran into some issues with how we would dynamically allocate memory, but we were able to figure it out using kmalloc and the associated package. Then we ran into some trouble with the producer thread not ending correctly. It was stuck in an infinite loop because we had the for each loop inside of the while loop that was supposed to check if it was ended. After that was fixed, the ending function was sometimes trying to send a stop signal to the producer thread even when it was already complete. We were able to fix this by correcting the producer function not to end before it is triggered to. The consumer thread 0 seems to take up a lot of the tasks sometimes and we are not sure why it gets so much priority but the other ones join in too sometimes. We also made some quick fixes to make sure that the timing was not being rounded every time so that we had a more accurate final count. Thank you for reading!
