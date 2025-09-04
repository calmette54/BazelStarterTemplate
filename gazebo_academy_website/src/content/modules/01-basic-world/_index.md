---
title: "Module 1: Basic World Creation"
description: "Master the fundamentals of Gazebo simulation with SDF worlds, entity-component systems, and basic model creation"
type: "module"
layout: "module"
weight: 10
tier: "free"
duration: "4-6 hours"
difficulty: "Beginner"
topics: ["SDF", "World Creation", "Entity-Component-System", "Basic Models"]
prerequisites: []
learning_objectives:
  - "Understand SDF world format and structure"
  - "Learn Gazebo's entity-component-system architecture"
  - "Create and manipulate basic simulation worlds"
  - "Inspect world state and manage entities"
  - "Configure essential system plugins"
code_examples: 4
hands_on_exercises: 6
---

# Module 1: Basic World Creation

Welcome to your journey into Gazebo simulation! This foundational module introduces you to the core concepts that power modern robotics simulation.

## 🎯 Module Overview

In this comprehensive introduction, you'll build solid foundations in Gazebo's architecture and learn to create your first simulation worlds. By the end of this module, you'll understand how Gazebo organizes simulation data and be ready to create your own interactive environments.

## 📋 What You'll Accomplish

By completing this module, you will:

✅ **Master SDF Format** - Understand the XML-based Simulation Description Format used to define worlds and models

✅ **Grasp ECS Architecture** - Learn how Gazebo's Entity-Component-System organizes simulation data efficiently  

✅ **Create Interactive Worlds** - Build your first simulation environments with physics, lighting, and models

✅ **Inspect Simulation State** - Use tools and code to examine entities, components, and system behaviors

✅ **Configure System Plugins** - Set up essential plugins that power physics, user commands, and scene broadcasting

## 🏗️ Module Structure

### Lesson 1.1: Understanding SDF Worlds
**Duration**: 60-90 minutes

Dive deep into the Simulation Description Format (SDF) that defines every Gazebo world. Learn the XML structure, essential elements, and best practices for world creation.

**Topics Covered:**
- SDF version compatibility and standards
- World element structure and properties  
- Physics configuration and timestep settings
- Plugin integration and system setup

**Hands-On Exercise**: Create your first SDF world file from scratch

### Lesson 1.2: Entity-Component-System Architecture  
**Duration**: 45-60 minutes

Understand Gazebo's modern ECS architecture that makes simulations both flexible and performant. Learn how entities, components, and systems work together.

**Topics Covered:**
- Entity identification and lifecycle
- Component data organization
- System processing and update loops
- Query patterns and entity inspection

**Code Example**: Build a world inspector that examines entities and components

### Lesson 1.3: Essential System Plugins
**Duration**: 75-90 minutes

Explore the core plugins that every Gazebo world needs to function properly. Learn about physics simulation, user interaction, and scene management.

**Topics Covered:**
- Physics system configuration and engines
- UserCommands for runtime interaction
- SceneBroadcaster for visualization  
- Plugin loading and parameter passing

**Hands-On Exercise**: Configure a world with optimized plugin settings

### Lesson 1.4: Basic Model Creation and Management
**Duration**: 90-120 minutes

Create your first 3D models and learn to position, orient, and manage them within simulation worlds. Understand the relationship between world and model SDF.

**Topics Covered:**
- Model SDF structure and elements
- Link and joint definitions
- Collision vs visual geometry  
- Model spawning and removal

**Code Example**: Dynamic model spawner application

### Lesson 1.5: World State Inspection and Debugging
**Duration**: 60-75 minutes

Learn essential debugging and inspection techniques to understand what's happening inside your simulation. Master tools for troubleshooting and optimization.

**Topics Covered:**
- Entity queries and component inspection
- Simulation timing and performance monitoring
- Debug visualization techniques
- Common troubleshooting approaches

**Hands-On Exercise**: Build a comprehensive world debugging tool

## 💻 Code Examples

This module includes 4 complete C++ programs that demonstrate core concepts:

### 1. Basic World Demo (`basic_world_demo.cc`)
A comprehensive simulation that loads SDF worlds, inspects entities, and demonstrates the simulation loop with custom systems.

```cpp
// Key features demonstrated:
- Server configuration and startup
- Custom system integration  
- Entity-component queries
- Simulation timing control
```

### 2. Interactive World Viewer (`world_viewer.cc`)
An interactive GUI application for exploring world contents, entity relationships, and component data in real-time.

### 3. SDF Validator (`sdf_validator.cc`)  
A utility tool for validating SDF world files, checking syntax, and reporting common configuration issues.

### 4. Launch Configurator (`launch_configurator.cc`)
Advanced world launcher with customizable settings, plugin configurations, and runtime parameter adjustment.

## 🔧 Hands-On Exercises

### Exercise 1.1: Your First SDF World
Create a basic world with ground plane, lighting, and simple geometric objects. Practice SDF syntax and world configuration.

### Exercise 1.2: Physics Configuration  
Experiment with different physics engines (DART, Bullet, TPE) and observe their behavioral differences.

### Exercise 1.3: Custom System Plugin
Build a simple system that monitors simulation state and reports entity counts and performance metrics.

### Exercise 1.4: Model Positioning Challenge
Create a world with precise model placement using coordinate systems and reference frames.

### Exercise 1.5: Debug Visualization
Implement visual debugging aids that help you understand entity relationships and component data.

### Exercise 1.6: Launch Automation
Create parameterized launch scripts that allow easy world configuration and plugin selection.

## 🎓 Assessment & Progression

**Knowledge Check**: Interactive quiz covering SDF structure, ECS architecture, and system plugin concepts

**Practical Assessment**: Build a complete simulation world that demonstrates all learned concepts

**Progression Criteria**: Complete all exercises and demonstrate understanding through the practical assessment

## 🔗 What's Next?

Upon mastering basic world creation, you'll be ready for **Module 2: Physics Simulation**, where you'll dive deep into collision detection, joint constraints, and material properties that bring your worlds to life.

---

*This module provides the essential foundation for everything that follows in the GazeboMaster Academy curriculum. Take your time, experiment with the examples, and don't hesitate to revisit concepts as needed.*