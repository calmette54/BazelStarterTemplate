# The project's whole system environment: `devenv shell` provides every tool
# needed to build, test and run it.
{pkgs, ...}: {
  packages = [
    pkgs.git
  ];

  enterShell = ''
    git --version
  '';

  # `devenv test` runs this: one line per tool the project relies on.
  enterTest = ''
    git --version
  '';
}
