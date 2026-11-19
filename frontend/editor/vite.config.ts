import { defineConfig } from 'vite'

export default defineConfig({
  base: './',
  build: {
    assetsDir: '.',
    assetsInlineLimit: 100_000_000,
    chunkSizeWarningLimit: 4_000,
    rolldownOptions: {
      output: {
        entryFileNames: 'editor.js',
        codeSplitting: false,
        assetFileNames: (assetInfo) =>
          assetInfo.names?.some((name) => name.endsWith('.css'))
            ? 'editor.css'
            : '[name][extname]',
      },
    },
  },
})
