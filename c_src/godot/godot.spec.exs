module Godot

interface [CNode]

state_type "MyState"

spec init(state, arguments :: [string]) :: {:ok :: label, state, code :: int}
                                  | {:fail :: label, state, result :: string}
spec iteration(state, delta :: int) :: {:ok :: label, state, code :: int}
                                  | {:fail :: label, state, result :: string}
spec call(state, method :: string)  :: {:ok :: label, state, type :: int, result_string :: string}
                                  | {:fail :: label, state, result :: string}
