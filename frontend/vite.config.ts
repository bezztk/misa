import react from '@vitejs/plugin-react'
import { tanstackRouter } from '@tanstack/router-plugin/vite'
import { defineConfig } from 'vite'

export default defineConfig({
  plugins: [tanstackRouter({ target: 'react' }), react()],
  server: {
    proxy: {
      '/api': 'http://127.0.0.1:3000',
    },
  },
})
