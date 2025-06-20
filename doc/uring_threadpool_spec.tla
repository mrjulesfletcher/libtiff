---- MODULE uring_threadpool_spec ----
EXTENDS Naturals, Sequences

CONSTANTS Buffers, Workers, RingDepth, PoolMax

VARIABLES queue, inUse, ring, completed

Init == /\ queue = << >>
         /\ inUse = {}
         /\ ring = << >>
         /\ completed = {}

Submit(buf) == /\ buf \in Buffers
                /\ Len(queue) < PoolMax
                /\ queue' = Append(queue, buf)
                /\ UNCHANGED <<inUse, ring, completed>>

StartWork(buf) == /\ Len(queue) > 0
                   /\ buf = Head(queue)
                   /\ queue' = Tail(queue)
                   /\ inUse' = inUse \cup {buf}
                   /\ UNCHANGED <<ring, completed>>

FinishWork(buf) == /\ buf \in inUse
                    /\ Len(ring) < RingDepth
                    /\ inUse' = inUse \ {buf}
                    /\ ring' = Append(ring, buf)
                    /\ UNCHANGED <<queue, completed>>

RingComplete(buf) == /\ Len(ring) > 0
                       /\ buf = Head(ring)
                       /\ ring' = Tail(ring)
                       /\ completed' = completed \cup {buf}
                       /\ UNCHANGED <<queue, inUse>>

Next == \E buf \in Buffers :
            Submit(buf)
            \/ StartWork(buf)
            \/ FinishWork(buf)
            \/ RingComplete(buf)

Spec == Init /\ [][Next]_<<queue, inUse, ring, completed>>

InvNoOverlap == \A buf \in Buffers : ~(buf \in inUse /\ buf \in ring)
InvQueueBound == Len(queue) <= PoolMax
InvRingBound == Len(ring) <= RingDepth

THEOREM Spec => [](InvNoOverlap /\ InvQueueBound /\ InvRingBound)
====
