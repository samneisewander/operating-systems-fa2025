# Project 02: Simple Message Queue

This is [Project 02] of [CSE.30341.FA25].

## Students

- Sam Neisewander (sneisewa@nd.edu)
- Leo Molina (lmolina3@nd.edu)

## Video

[Reflection Video](https://www.youtube.com/watch?v=qIHVa9ap_vY)

## Brainstorming

The following are questions that should help you in thinking about how to
approach implementing [Project 02].  For this project, responses to these
brainstorming questions **are not required**.

### Request

1. What data must be allocated and deallocated for each `Request` structure?

>  The message body, the url, and the HTTP method.

2. How do you use [libcurl] to perform a simple [HTTP] request?

### Queue

1. What data must be allocated and deallocated for each `Queue` structure?

> Need to allocate and deallocate a new queue struct.

2. How will you implement **mutual exclusion**?

> Going to use one mutex to control access to the list. This is important because both pushing and popping operations modify the shared `queue->size` attribute, so if the consumer pops and the producer pushes at the same time, there is a race condition. 

3. How will you implement **signaling**?

> Going to use one condition variable to signal when something has been produced. This implementation should only need one condition variable because the `Requests` queue is unbounded, so the producers will *never have to wait for consumers when they wish to push*. They will simply grab the lock, push, and release the lock. Consumers, however, need to sleep until there is at least one thing on the queue to consume, so we need to give the producer thread a condition variable with which to signal that event.

4. What are the **critical sections**?

> - The portions of queue_push and queue_pop that modify `queue->size`.
> - The portion of queue_push that modifies `queue->head`.
> - The portion of queue_pop that modifies `queue->tail`.
> - The portion of queue_shutdown that modifies `queue->running`.

5. What is the purpose of `queue_shutdown`?

> Shutdown needs to destroy the queue structure, and will get called by `smq_shutdown`.

6. How will the `timeout` in `queue_pop` be used?

> If the pusher thread tries to pop a new message to send to the server and there is nothing there, this can be for one of two reasons:
> 1. The user has not sent any messages in a while.
> 2. The user has exited the application.
> In the first case, we don't want the pusher thread to kill itself, because at some point the user may write more messages, and those still need to get sent. So in this case we should just retry the pop and wait until the queue is filled, or we time out again.
> In the second case, we want to shut down the pusher thread so that it can rejoin the main thread and exit gracefully. They way we can tell that the application has exited is by checking the `queue->running` flag, which should be set to false when the user exits the application. In this case, 

### Client

1. What data must be allocated and deallocated for each `SMQ` structure?

2. What should happen when the user **publishes** a message?

3. What should happen when the user **retrieves** a message?

4. What should happen when the user **subscribes** to a topic?

5. What should happen when the user **unsubscribes** to a topic?

6. How many internal **threads** are required?

7. What is the purpose of each internal **thread**?

8. When does each **thread** get created and joined?

9. What `SMQ` attribute needs to be **protected** from **concurrent** access?

10. How is the client **shutdown**?

## Errata

> Describe any known errors, bugs, or deviations from the requirements.

## Acknowledgments

> List anyone you collaborated with or received help from (including TAs, other
students, and AI tools)

[Project 02]: https://pnutz.h4x0r.space/courses/cse.30341.fa25/project02.html
[CSE.30341.FA25]: https://pnutz.h4x0r.space/courses/cse.30341.fa25/
[libcurl]: https://curl.se/libcurl/c/
[HTTP]: https://en.wikipedia.org/wiki/HTTP
