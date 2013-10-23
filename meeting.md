# Grappa Meeting Agenda
--------
## Oct 16
### Open sourcing
- Technical TODOs we came up with, priority levels
- Timeline: in time for Sandia (?)
- Discuss goals, plan for publicizing, web presence

### Individual updates
- Holt
  - global pointer code-gen pass prototype working, just need to link against actual Grappa delegates
  - There's a possibility of writing a paper with Michael Ferguson. We should decide if we're going to do it and get him on board soon.
  - working on getting CMake to stably build Grappa on OSX/Linux
- Myers
  - Thinking about how to evaluate queries more efficiently (less data movement)
  - Figuring out which if any compiler infrastructure will be useful for the C/Grappa backend of the Datalog compiler (it is currently homegrown)
- Nelson
  - I continue to work on my thesis, and am figuring out what to include in my evaluation.
- Vincent
  - Connected Components & Grappa training wheels
- Jake
  - Graph Generators

## Oct 23
### Updates
- Holt
  - Fixing SharedMessagePool issues so we can merge new graph stuff
  - CMake build working (with dependent projects, too). Waiting on other fixes before polishing and showing to everyone
  - Planning to do new prototype Set implementation based on "Transactional Boosting" work
  