---- MODULE threadpool_spec ----
EXTENDS Naturals, Sequences

CONSTANTS Workers, MaxQueue

VARIABLES queue, completed

Init == /\ queue = << >>
         /\ completed = {}

Submit(task) == /\ Len(queue) < MaxQueue
                 /\ queue' = Append(queue, task)
                 /\ UNCHANGED completed

Execute(task) == /\ Len(queue) > 0
                  /\ task = Head(queue)
                  /\ queue' = Tail(queue)
                  /\ completed' = completed \cup {task}

Next == \E task \in Nat : Submit(task) \/ Execute(task)

Spec == Init /\ [][Next]_<<queue, completed>>

Invariant == Len(queue) <= MaxQueue

THEOREM Spec => []Invariant
====
