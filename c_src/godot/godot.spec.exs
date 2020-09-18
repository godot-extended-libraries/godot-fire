module Godot

interface [CNode]

state_type "MyState"

spec init(state, arguments :: [string]) :: {:ok :: label, state, code :: int}
                                  | {:fail :: label, state, result :: string}
spec iteration(state, delta :: int) :: {:ok :: label, state, code :: int}
                                  | {:fail :: label, state, result :: string}
@type return_type :string | :float | :int | :bool | String.t
@type return_value String.t | float | integer | boolean
@spec call(state, method :: String.t) :: {:ok, return_type, return_value}
