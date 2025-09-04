const { fontFamily } = require('tailwindcss/defaultTheme')

/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    './src/**/*.{html,js,md}',
    './hugo.yaml',
  ],
  theme: {
    extend: {
      // Custom color palette for educational platform
      colors: {
        // Primary brand colors - inspired by robotics and technology
        primary: {
          50: '#eff6ff',
          100: '#dbeafe', 
          200: '#bfdbfe',
          300: '#93c5fd',
          400: '#60a5fa',
          500: '#3b82f6', // Main brand blue
          600: '#2563eb',
          700: '#1d4ed8',
          800: '#1e40af',
          900: '#1e3a8a',
        },
        // Secondary accent colors - complementary orange for CTAs
        secondary: {
          50: '#fff7ed',
          100: '#ffedd5',
          200: '#fed7aa',
          300: '#fdba74',
          400: '#fb923c',
          500: '#f97316', // Main accent orange
          600: '#ea580c',
          700: '#c2410c',
          800: '#9a3412',
          900: '#7c2d12',
        },
        // Success/completion colors - green palette
        success: {
          50: '#f0fdf4',
          100: '#dcfce7',
          200: '#bbf7d0',
          300: '#86efac',
          400: '#4ade80',
          500: '#22c55e',
          600: '#16a34a',
          700: '#15803d',
          800: '#166534',
          900: '#14532d',
        },
        // Warning/premium colors - amber palette
        premium: {
          50: '#fffbeb',
          100: '#fef3c7',
          200: '#fde68a',
          300: '#fcd34d',
          400: '#fbbf24',
          500: '#f59e0b',
          600: '#d97706',
          700: '#b45309',
          800: '#92400e',
          900: '#78350f',
        },
        // Neutral grays optimized for reading
        gray: {
          50: '#f9fafb',
          100: '#f3f4f6',
          200: '#e5e7eb',
          300: '#d1d5db',
          400: '#9ca3af',
          500: '#6b7280',
          600: '#4b5563',
          700: '#374151',
          800: '#1f2937',
          900: '#111827',
        },
        // Code syntax highlighting colors
        code: {
          bg: '#1e293b',
          text: '#e2e8f0',
          comment: '#64748b',
          keyword: '#38bdf8',
          string: '#84cc16',
          function: '#f59e0b',
          variable: '#e879f9',
        }
      },
      
      // Typography system optimized for educational content
      fontFamily: {
        sans: ['Inter var', 'Inter', ...fontFamily.sans],
        mono: ['JetBrains Mono', 'Fira Code', ...fontFamily.mono],
        display: ['Inter var', 'Inter', ...fontFamily.sans],
      },
      
      // Extended spacing for educational layouts
      spacing: {
        '18': '4.5rem',
        '88': '22rem',
        '128': '32rem',
      },
      
      // Custom animations for interactive elements
      animation: {
        'fade-in': 'fadeIn 0.5s ease-in-out',
        'slide-up': 'slideUp 0.3s ease-out',
        'pulse-soft': 'pulseSoft 2s cubic-bezier(0.4, 0, 0.6, 1) infinite',
        'progress': 'progress 0.5s ease-out',
      },
      
      keyframes: {
        fadeIn: {
          '0%': { opacity: '0' },
          '100%': { opacity: '1' },
        },
        slideUp: {
          '0%': { transform: 'translateY(10px)', opacity: '0' },
          '100%': { transform: 'translateY(0)', opacity: '1' },
        },
        pulseSoft: {
          '0%, 100%': { opacity: '1' },
          '50%': { opacity: '0.8' },
        },
        progress: {
          '0%': { width: '0%' },
          '100%': { width: 'var(--progress-width)' },
        }
      },
      
      // Box shadows for depth and hierarchy
      boxShadow: {
        'soft': '0 2px 15px -3px rgba(0, 0, 0, 0.07), 0 10px 20px -2px rgba(0, 0, 0, 0.04)',
        'medium': '0 4px 25px -5px rgba(0, 0, 0, 0.1), 0 25px 50px -12px rgba(0, 0, 0, 0.25)',
        'large': '0 20px 25px -5px rgba(0, 0, 0, 0.1), 0 25px 50px -12px rgba(0, 0, 0, 0.25)',
        'code': '0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06), inset 0 1px 0 rgba(255, 255, 255, 0.1)',
      },
      
      // Border radius for modern, friendly design
      borderRadius: {
        'xl': '0.75rem',
        '2xl': '1rem',
        '3xl': '1.5rem',
      },
      
      // Custom grid templates for course layouts
      gridTemplateColumns: {
        'module': '1fr 300px',
        'course-grid': 'repeat(auto-fit, minmax(300px, 1fr))',
      },
      
      // Typography scale optimized for learning content
      fontSize: {
        'xs': ['0.75rem', { lineHeight: '1rem' }],
        'sm': ['0.875rem', { lineHeight: '1.25rem' }],
        'base': ['1rem', { lineHeight: '1.5rem' }],
        'lg': ['1.125rem', { lineHeight: '1.75rem' }],
        'xl': ['1.25rem', { lineHeight: '1.75rem' }],
        '2xl': ['1.5rem', { lineHeight: '2rem' }],
        '3xl': ['1.875rem', { lineHeight: '2.25rem' }],
        '4xl': ['2.25rem', { lineHeight: '2.5rem' }],
        '5xl': ['3rem', { lineHeight: '1' }],
        '6xl': ['3.75rem', { lineHeight: '1' }],
        '7xl': ['4.5rem', { lineHeight: '1' }],
        '8xl': ['6rem', { lineHeight: '1' }],
        '9xl': ['8rem', { lineHeight: '1' }],
      },
      
      // Content width constraints for optimal reading
      maxWidth: {
        '8xl': '88rem',
        '9xl': '96rem',
        'content': '65ch', // Optimal line length for reading
      },
    },
  },
  plugins: [
    require('@tailwindcss/forms')({
      strategy: 'class', // Only add form styles when using form-* classes
    }),
    require('@tailwindcss/typography')({
      target: 'modern', // Use modern CSS features
    }),
    
    // Custom plugin for educational components
    function({ addComponents, theme }) {
      addComponents({
        // Button styles
        '.btn-primary': {
          backgroundColor: theme('colors.primary.500'),
          color: theme('colors.white'),
          padding: `${theme('spacing.3')} ${theme('spacing.6')}`,
          borderRadius: theme('borderRadius.lg'),
          fontWeight: theme('fontWeight.semibold'),
          fontSize: theme('fontSize.base[0]'),
          lineHeight: theme('fontSize.base[1].lineHeight'),
          textAlign: 'center',
          textDecoration: 'none',
          display: 'inline-flex',
          alignItems: 'center',
          justifyContent: 'center',
          transition: 'all 0.2s ease-in-out',
          boxShadow: theme('boxShadow.soft'),
          '&:hover': {
            backgroundColor: theme('colors.primary.600'),
            transform: 'translateY(-1px)',
            boxShadow: theme('boxShadow.medium'),
          },
          '&:focus': {
            outline: '2px solid transparent',
            outlineOffset: '2px',
            '--tw-ring-offset-shadow': '0 0 0 2px #fff',
            '--tw-ring-shadow': `0 0 0 4px ${theme('colors.primary.500')}25`,
            boxShadow: 'var(--tw-ring-offset-shadow), var(--tw-ring-shadow), var(--tw-shadow, 0 0 #0000)',
          },
        },
        
        '.btn-secondary': {
          backgroundColor: 'transparent',
          color: theme('colors.gray.700'),
          padding: `${theme('spacing.3')} ${theme('spacing.6')}`,
          borderRadius: theme('borderRadius.lg'),
          fontWeight: theme('fontWeight.semibold'),
          fontSize: theme('fontSize.base[0]'),
          lineHeight: theme('fontSize.base[1].lineHeight'),
          textAlign: 'center',
          textDecoration: 'none',
          display: 'inline-flex',
          alignItems: 'center',
          justifyContent: 'center',
          transition: 'all 0.2s ease-in-out',
          border: `2px solid ${theme('colors.gray.300')}`,
          '&:hover': {
            backgroundColor: theme('colors.gray.50'),
            borderColor: theme('colors.gray.400'),
            transform: 'translateY(-1px)',
          },
        },
        
        // Card styles for modules and content
        '.card': {
          backgroundColor: theme('colors.white'),
          borderRadius: theme('borderRadius.2xl'),
          padding: theme('spacing.6'),
          boxShadow: theme('boxShadow.soft'),
          border: `1px solid ${theme('colors.gray.200')}`,
          transition: 'all 0.2s ease-in-out',
          '&:hover': {
            boxShadow: theme('boxShadow.medium'),
            transform: 'translateY(-2px)',
          },
        },
        
        // Code block styling
        '.code-block': {
          backgroundColor: theme('colors.code.bg'),
          color: theme('colors.code.text'),
          borderRadius: theme('borderRadius.lg'),
          padding: theme('spacing.4'),
          fontFamily: theme('fontFamily.mono').join(', '),
          fontSize: theme('fontSize.sm[0]'),
          lineHeight: theme('fontSize.sm[1].lineHeight'),
          overflow: 'auto',
          boxShadow: theme('boxShadow.code'),
        },
        
        // Progress indicators
        '.progress-bar': {
          width: '100%',
          height: theme('spacing.2'),
          backgroundColor: theme('colors.gray.200'),
          borderRadius: theme('borderRadius.full'),
          overflow: 'hidden',
          '& .progress-fill': {
            height: '100%',
            backgroundColor: theme('colors.success.500'),
            borderRadius: theme('borderRadius.full'),
            transition: 'width 0.5s ease-out',
          },
        },
        
        // Pricing card styles
        '.pricing-card': {
          backgroundColor: theme('colors.white'),
          borderRadius: theme('borderRadius.2xl'),
          padding: theme('spacing.8'),
          boxShadow: theme('boxShadow.soft'),
          border: `2px solid ${theme('colors.gray.200')}`,
          position: 'relative',
          transition: 'all 0.3s ease-in-out',
          
          '&.pro': {
            borderColor: theme('colors.primary.500'),
            boxShadow: theme('boxShadow.medium'),
            transform: 'scale(1.05)',
          },
          
          '&.enterprise': {
            borderColor: theme('colors.premium.500'),
            background: `linear-gradient(135deg, ${theme('colors.white')} 0%, ${theme('colors.premium.50')} 100%)`,
          },
        },
      })
    }
  ],
}