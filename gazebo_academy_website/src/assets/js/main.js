// GazeboMaster Academy - Main JavaScript
// Built with Alpine.js for reactive components

import Alpine from 'alpinejs'

// Initialize Alpine.js
window.Alpine = Alpine

// Global site configuration
const siteConfig = {
  apiEndpoint: '/api',
  stripePublishableKey: process.env.STRIPE_PUBLISHABLE_KEY || 'pk_test_...',
  version: '1.0.0'
}

// Navigation Component
Alpine.data('navigation', () => ({
  isOpen: false,
  isScrolled: false,
  
  init() {
    // Handle scroll events for navbar styling
    window.addEventListener('scroll', () => {
      this.isScrolled = window.scrollY > 10
    })
    
    // Close mobile menu on route change
    window.addEventListener('popstate', () => {
      this.isOpen = false
    })
  },
  
  toggleMenu() {
    this.isOpen = !this.isOpen
    document.body.classList.toggle('overflow-hidden', this.isOpen)
  },
  
  closeMenu() {
    this.isOpen = false
    document.body.classList.remove('overflow-hidden')
  }
}))

// Module Progress Tracking
Alpine.data('moduleProgress', (moduleId) => ({
  progress: 0,
  completed: false,
  timeSpent: 0,
  
  init() {
    this.loadProgress()
    this.startTimeTracking()
  },
  
  loadProgress() {
    const saved = localStorage.getItem(`module_${moduleId}_progress`)
    if (saved) {
      const data = JSON.parse(saved)
      this.progress = data.progress || 0
      this.completed = data.completed || false
      this.timeSpent = data.timeSpent || 0
    }
  },
  
  updateProgress(newProgress) {
    this.progress = Math.max(this.progress, newProgress)
    this.completed = this.progress >= 100
    this.saveProgress()
  },
  
  markLessonComplete(lessonIndex, totalLessons) {
    const lessonProgress = ((lessonIndex + 1) / totalLessons) * 100
    this.updateProgress(lessonProgress)
  },
  
  saveProgress() {
    localStorage.setItem(`module_${moduleId}_progress`, JSON.stringify({
      progress: this.progress,
      completed: this.completed,
      timeSpent: this.timeSpent,
      lastAccessed: new Date().toISOString()
    }))
  },
  
  startTimeTracking() {
    let startTime = Date.now()
    let interval = setInterval(() => {
      this.timeSpent += 1
      if (this.timeSpent % 60 === 0) { // Save every minute
        this.saveProgress()
      }
    }, 1000)
    
    // Stop tracking when user leaves page
    window.addEventListener('beforeunload', () => {
      clearInterval(interval)
      this.saveProgress()
    })
  },
  
  formatTime() {
    const hours = Math.floor(this.timeSpent / 3600)
    const minutes = Math.floor((this.timeSpent % 3600) / 60)
    return hours > 0 ? `${hours}h ${minutes}m` : `${minutes}m`
  }
}))

// Code Playground Component
Alpine.data('codePlayground', () => ({
  code: '',
  output: '',
  isRunning: false,
  language: 'cpp',
  examples: {
    cpp: `#include <gz/sim/Server.hh>
#include <gz/common/Console.hh>

int main() {
    gzmsg << "Hello GazeboMaster Academy!" << std::endl;
    return 0;
}`,
    python: `import gz.sim as sim
import gz.common as common

def main():
    common.Console.msg("Hello GazeboMaster Academy!")

if __name__ == "__main__":
    main()`,
    sdf: `<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="hello_world">
    <plugin filename="gz-sim-physics-system" 
            name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" 
            name="gz::sim::systems::UserCommands"/>
    
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane><normal>0 0 1</normal></plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane><normal>0 0 1</normal></plane>
          </geometry>
        </visual>
      </link>
    </model>
  </world>
</sdf>`
  },
  
  init() {
    this.code = this.examples[this.language]
  },
  
  changeLanguage(newLanguage) {
    this.language = newLanguage
    this.code = this.examples[newLanguage]
    this.output = ''
  },
  
  async runCode() {
    this.isRunning = true
    this.output = 'Running code...\n'
    
    try {
      // Simulate code execution (in real implementation, this would call a backend API)
      await new Promise(resolve => setTimeout(resolve, 1500))
      
      if (this.language === 'cpp') {
        this.output = `Compilation successful!
Output:
Hello GazeboMaster Academy!

Process completed in 0.42 seconds.`
      } else if (this.language === 'sdf') {
        this.output = `SDF Validation Result:
✅ Valid SDF file
✅ All required plugins found
✅ Model structure correct

World 'hello_world' ready for simulation.`
      } else {
        this.output = `Python execution successful!
Hello GazeboMaster Academy!

Execution time: 0.12 seconds`
      }
    } catch (error) {
      this.output = `Error: ${error.message}`
    } finally {
      this.isRunning = false
    }
  },
  
  copyCode() {
    navigator.clipboard.writeText(this.code).then(() => {
      this.showToast('Code copied to clipboard!')
    })
  },
  
  showToast(message) {
    // Simple toast notification
    const toast = document.createElement('div')
    toast.className = 'fixed bottom-4 right-4 bg-green-500 text-white px-4 py-2 rounded-lg z-50'
    toast.textContent = message
    document.body.appendChild(toast)
    
    setTimeout(() => {
      toast.remove()
    }, 3000)
  }
}))

// Search Functionality
Alpine.data('search', () => ({
  query: '',
  results: [],
  isSearching: false,
  showResults: false,
  
  async performSearch() {
    if (this.query.length < 2) {
      this.results = []
      this.showResults = false
      return
    }
    
    this.isSearching = true
    
    try {
      // Simulate search API call
      await new Promise(resolve => setTimeout(resolve, 300))
      
      // Mock search results
      this.results = [
        {
          title: 'Module 1: Basic World Creation',
          excerpt: 'Learn the fundamentals of SDF worlds and entity-component systems...',
          url: '/modules/01-basic-world/',
          type: 'module'
        },
        {
          title: 'Physics Engine Comparison',
          excerpt: 'Understanding DART, Bullet, and TPE physics engines...',
          url: '/modules/02-physics-simulation/',
          type: 'lesson'
        },
        {
          title: 'SDF World Format Reference',
          excerpt: 'Complete guide to Simulation Description Format...',
          url: '/docs/sdf-reference/',
          type: 'documentation'
        }
      ].filter(item => 
        item.title.toLowerCase().includes(this.query.toLowerCase()) ||
        item.excerpt.toLowerCase().includes(this.query.toLowerCase())
      )
      
      this.showResults = true
    } finally {
      this.isSearching = false
    }
  },
  
  selectResult(result) {
    window.location.href = result.url
    this.clearSearch()
  },
  
  clearSearch() {
    this.query = ''
    this.results = []
    this.showResults = false
  }
}))

// Subscription Management
Alpine.data('subscription', () => ({
  currentPlan: 'free',
  isLoading: false,
  
  init() {
    this.loadUserData()
  },
  
  loadUserData() {
    // In real implementation, load from authentication service
    const userData = localStorage.getItem('user_data')
    if (userData) {
      const user = JSON.parse(userData)
      this.currentPlan = user.subscription || 'free'
    }
  },
  
  async upgradeToPro() {
    this.isLoading = true
    
    try {
      // In real implementation, integrate with Stripe
      const response = await fetch(`${siteConfig.apiEndpoint}/create-checkout-session`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          plan: 'pro',
          price_id: 'price_pro_monthly'
        })
      })
      
      const { checkout_url } = await response.json()
      window.location.href = checkout_url
    } catch (error) {
      console.error('Upgrade error:', error)
      alert('Sorry, there was an error processing your upgrade. Please try again.')
    } finally {
      this.isLoading = false
    }
  },
  
  async contactSales() {
    // Simple contact form handling
    const email = prompt('Please enter your email address for Enterprise pricing:')
    if (email && this.isValidEmail(email)) {
      try {
        await fetch(`${siteConfig.apiEndpoint}/contact-sales`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({
            email,
            plan: 'enterprise',
            source: 'pricing_page'
          })
        })
        
        alert('Thank you! Our sales team will contact you within 24 hours.')
      } catch (error) {
        console.error('Contact error:', error)
      }
    }
  },
  
  isValidEmail(email) {
    return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)
  }
}))

// Course Dashboard
Alpine.data('dashboard', () => ({
  stats: {
    modulesCompleted: 0,
    totalModules: 8,
    hoursLearned: 0,
    currentStreak: 0
  },
  recentActivity: [],
  
  init() {
    this.loadStats()
    this.loadRecentActivity()
  },
  
  loadStats() {
    let totalProgress = 0
    let totalTime = 0
    
    for (let i = 1; i <= 8; i++) {
      const progress = localStorage.getItem(`module_${i}_progress`)
      if (progress) {
        const data = JSON.parse(progress)
        if (data.completed) {
          this.stats.modulesCompleted++
        }
        totalTime += data.timeSpent || 0
      }
    }
    
    this.stats.hoursLearned = Math.round(totalTime / 3600 * 10) / 10
    this.calculateStreak()
  },
  
  calculateStreak() {
    // Simple streak calculation based on daily activity
    const today = new Date().toDateString()
    const lastActivity = localStorage.getItem('last_activity_date')
    
    if (lastActivity === today) {
      this.stats.currentStreak = parseInt(localStorage.getItem('current_streak') || '1')
    } else {
      this.stats.currentStreak = 0
    }
    
    // Update activity tracking
    localStorage.setItem('last_activity_date', today)
    localStorage.setItem('current_streak', this.stats.currentStreak.toString())
  },
  
  loadRecentActivity() {
    this.recentActivity = [
      {
        type: 'module_completed',
        title: 'Completed Module 2: Physics Simulation',
        date: new Date(Date.now() - 86400000).toLocaleDateString(),
        icon: '✅'
      },
      {
        type: 'lesson_started',
        title: 'Started Lesson: Joint Systems',
        date: new Date(Date.now() - 172800000).toLocaleDateString(),
        icon: '📖'
      }
    ]
  },
  
  get progressPercentage() {
    return Math.round((this.stats.modulesCompleted / this.stats.totalModules) * 100)
  }
}))

// Pricing Calculator
Alpine.data('pricingCalculator', () => ({
  billingCycle: 'monthly',
  teamSize: 1,
  
  get proPrice() {
    return this.billingCycle === 'monthly' ? 29 : 290
  },
  
  get enterprisePrice() {
    const basePrice = this.billingCycle === 'monthly' ? 99 : 990
    return this.teamSize <= 25 ? basePrice : basePrice + Math.ceil((this.teamSize - 25) / 10) * 20
  },
  
  get savings() {
    if (this.billingCycle === 'annual') {
      return Math.round(((29 * 12) - 290) / (29 * 12) * 100)
    }
    return 0
  }
}))

// Initialize Alpine when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
  Alpine.start()
  
  // Add smooth scrolling for anchor links
  document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
      e.preventDefault()
      const target = document.querySelector(this.getAttribute('href'))
      if (target) {
        target.scrollIntoView({
          behavior: 'smooth',
          block: 'start'
        })
      }
    })
  })
  
  // Add copy button to code blocks
  document.querySelectorAll('pre code').forEach((codeBlock) => {
    const button = document.createElement('button')
    button.className = 'copy-button absolute top-2 right-2 p-2 text-gray-400 hover:text-white'
    button.innerHTML = '📋'
    button.title = 'Copy code'
    
    button.addEventListener('click', () => {
      navigator.clipboard.writeText(codeBlock.textContent).then(() => {
        button.innerHTML = '✅'
        setTimeout(() => {
          button.innerHTML = '📋'
        }, 2000)
      })
    })
    
    codeBlock.parentElement.style.position = 'relative'
    codeBlock.parentElement.appendChild(button)
  })
})

// Export for potential module usage
export { siteConfig }