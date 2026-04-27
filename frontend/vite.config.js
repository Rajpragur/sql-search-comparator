import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/executeQuery': 'http://127.0.0.1:5001',
      '/tables': 'http://127.0.0.1:5001',
      '/query': 'http://127.0.0.1:5001'
    }
  },
  build: {
    outDir: 'dist',
    emptyOutDir: true
  }
});
