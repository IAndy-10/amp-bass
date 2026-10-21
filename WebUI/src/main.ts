import '@fontsource/dm-sans/latin-400.css';
import '@fontsource/dm-sans/latin-500.css';
import '@fontsource/dm-sans/latin-600.css';
import '@fontsource/dm-sans/latin-600-italic.css';
import '@fontsource/dm-sans/latin-300.css';
import '@fontsource/dm-serif-display/latin-400-italic.css';
import './lib/styles/tokens.css';
import App from './App.svelte';
import { mount } from 'svelte';

const app = mount(App, { target: document.body });

export default app;
