import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'

// base: './' -> relative asset paths so the site works both at
// https://slcode777.github.io/gluco-family/ and at a custom root domain
// like https://flash.tondomaine.com without rebuilding.
export default defineConfig({
  base: './',
  plugins: [react(), tailwindcss()],
})
