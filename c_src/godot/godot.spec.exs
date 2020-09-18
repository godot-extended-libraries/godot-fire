module Godot

interface [CNode]

state_type "MyState"

spec init(state, arguments :: [string]) :: {:ok :: label, state, code :: int}
spec iteration(state, delta :: int) :: {:ok :: label, state, code :: int}
spec call(state, method :: string) :: {:ok :: label, state, result :: string}
