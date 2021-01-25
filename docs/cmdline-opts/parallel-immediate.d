Long: parallel-immediate
Help: Do not wait for multiplexing (with --parallel)
Added: 7.68.0
See-also: parallel parallel-max
Category: connection carl
---
When doing parallel transfers, this option will instruct carl that it should
rather prefer opening up more connections in parallel at once rather than
waiting to see if new transfers can be added as multiplexed streams on another
connection.
