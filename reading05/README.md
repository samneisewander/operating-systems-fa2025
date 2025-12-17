# Reading 05


# Devel

Wondering who should be in charge of freeing the malloced strings that get pushed into the queue. I think that:
- If you pop a string off the queue, you're in charge of freeing it
- If the queue gets deleted, it is responsible for cleaning up whatever was leftover
 
...so this function needs to march over all the queue nodes and free the strings.
```c
void queue_delete(Queue *q) {
    if (q) {
        free(q->data);
        free(q);
    }
}
```

Wondering why `thread_detach` is provided in `thread.h`. It might be that I'm supposed to detach the worker threads and have them continuously wait for something to be produced until the program terminates. But then how would you retrieve the failure count from the worker threads in the main thread? 

My initial thought was that each worker thread would count its own failures and return them to main when `thread_join` gets called. But then how would the worker thread know when to return? From its perspective, even though the work queue is empty *right now*, something might get added to it *in the future*.

After lecture I have learned that the **sentinel** is what signals to the worker threads that they should return.
