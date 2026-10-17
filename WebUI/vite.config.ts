import { defineConfig } from 'vite';
import { viteSingleFile } from 'vite-plugin-singlefile';
import { svelte, vitePreprocess } from '@sveltejs/vite-plugin-svelte';
import glsl from 'vite-plugin-glsl';

export default defineConfig({
    plugins: [
        ...(process.env.NODE_ENV === 'production' ? [viteSingleFile()] : []),
        svelte({ preprocess: vitePreprocess() }),
        glsl(),
    ],
    resolve: {
        alias: { $lib: decodeURIComponent(new URL('./src/lib', import.meta.url).pathname) },
    },
    build: {
        outDir: 'dist',
        emptyOutDir: true,
    },
    server: {
        middlewareMode: false,
    },
});
