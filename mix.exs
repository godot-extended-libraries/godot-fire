defmodule Godot.Mixfile do
  use Mix.Project

  def project do
    [
      app: :godot,
      compilers: [:unifex, :bundlex] ++ Mix.compilers,
      version: "0.1.0",
      deps: deps(),
      extra_applications: [:logger]
   ]
  end

  def application do
    [
      extra_applications: [:logger]
    ]
  end

  defp deps() do
    [
      {:unifex, "~> 0.3.1"} # add unifex to deps
    ]
  end
end