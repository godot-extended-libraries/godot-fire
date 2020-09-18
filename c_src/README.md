# Unifex Godot Engine

Launch.

```bash
export LD_LIBRARY_PATH=bin:$LD_LIBRARY_PATH
iex -S mix
require Unifex.CNode
{:ok, pid} = Unifex.CNode.start_link(:godot)
Unifex.CNode.call(pid, :init, [["godot", "-v"]])
Unifex.CNode.call(pid, :iteration, [1])
Unifex.CNode.call(pid, :call, ["get_node_count"])
Unifex.CNode.call(pid, :finish, [])
Unifex.CNode.call(pid, :init, [["godot", "-v"]])
Unifex.CNode.stop(pid)
```