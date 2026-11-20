import { defineConfig } from 'vite'

export default defineConfig({
  base: './',
  build: {
    assetsDir: '.',
    assetsInlineLimit: 100_000_000,
    rolldownOptions: {
      output: {
        entryFileNames: 'calendar.js',
        codeSplitting: false,
        assetFileNames: (assetInfo) =>
          assetInfo.names?.some((name) => name.endsWith('.css'))
            ? 'calendar.css'
            : '[name][extname]',
      },
    },
  },
})
