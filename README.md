# Welcome to TaskDroid!
---
TaskDroid is an Android App static analysis tool based on Android multi-tasking mechanism.

<!-- The semantics of Android multi-tasking mechanism is in [Understanding Android Mutitasking Mechanism](https://github.com/Jinlong-He/TaskDroid/blob/master/semantics.md). -->
<!-- --- -->
# How to build
```
python3 build.py
```
# How to run
```
python3 run.py -i <input file> -o <output file> {-b|-r <reach file>} [-e {nfa|nuxmv}] [-h <height>] [-k <number>] [-t <timeout>]
```
-i <input file>: To specify the input apk file,
-o <output file>: To specify the output file,
-b: To specify the task as analyzing the task unboundedness problem,
-r <reach file>： To specify the task as analyzing the configuration reachability problem, use reach file to describe the configurations to analyze,
-e {nfa|nuxmv}: To specify the engine for analyzing the configuration reachability problem,
-h <height>: To set the bound of the height of stack, the default value is 6,
-k <number>: To set the bound of the number of tasks, the default value is 2,
-t <timeout>: To set the timeout, the default value is 300.


