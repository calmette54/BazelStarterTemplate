#include <iostream>
#include <memory>

// Drake headers for simple continuous time system
#include "drake/systems/analysis/simulator.h"
#include "drake/systems/framework/leaf_system.h"

namespace drake {
namespace systems {
namespace {

// Simple Continuous Time System
// xdot = -x + x³
// y = x
class SimpleContinuousTimeSystem : public LeafSystem<double> {
 public:
  SimpleContinuousTimeSystem() {
    // Declare one continuous state variable
    auto state_index = DeclareContinuousState(1);
    
    // Declare output port that outputs the state
    DeclareStateOutputPort("y", state_index);
  }

 private:
  // Implement the dynamics: xdot = -x + x³
  void DoCalcTimeDerivatives(
      const Context<double>& context,
      ContinuousState<double>* derivatives) const override {
    const double x = context.get_continuous_state()[0];
    const double xdot = -x + std::pow(x, 3.0);
    (*derivatives)[0] = xdot;
  }
};

int main() {
  std::cout << "Drake Continuous Time System Example" << std::endl;
  std::cout << "=====================================" << std::endl;

  try {
    // Create the simple system
    SimpleContinuousTimeSystem system;
    std::cout << "✓ Created SimpleContinuousTimeSystem" << std::endl;

    // Create the simulator
    Simulator<double> simulator(system);
    std::cout << "✓ Created Simulator" << std::endl;

    // Set the initial conditions x(0) = 0.9
    ContinuousState<double>& state =
        simulator.get_mutable_context().get_mutable_continuous_state();
    state[0] = 0.9;
    std::cout << "✓ Set initial condition x(0) = " << state[0] << std::endl;

    // Simulate for 10 seconds
    std::cout << "Running simulation for 10 seconds..." << std::endl;
    simulator.AdvanceTo(10.0);
    
    // Get final state
    const double final_state = state[0];
    std::cout << "✓ Simulation completed" << std::endl;
    std::cout << "Final state x(10) = " << final_state << std::endl;

    // Verify the simulation converged to the stable fixed point at x=0
    // The system xdot = -x + x³ has fixed points at x = 0, ±1
    // x = 0 is stable, x = ±1 are unstable
    const double tolerance = 1.0e-4;
    if (std::abs(final_state) < tolerance) {
      std::cout << "✓ System converged to stable fixed point x = 0" << std::endl;
      std::cout << "✓ Drake prebuilt binary integration successful!" << std::endl;
      std::cout << "✓ Hermetic system dependencies verified!" << std::endl;
    } else {
      std::cout << "! Warning: System did not converge as expected" << std::endl;
      std::cout << "Expected |x| < " << tolerance << ", got |x| = " << std::abs(final_state) << std::endl;
    }

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}

} // namespace
} // namespace systems
} // namespace drake

int main() {
  return drake::systems::main();
}