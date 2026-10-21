/** Shared tooltip and context-menu state (spec §9 Tooltip / ContextMenu, §10). */

import { writable } from 'svelte/store';
import { nativeFunction } from '../../bridge/bridge';

export interface TooltipState {
    text: string;
    anchor: DOMRect | null;
}

export const tooltip = writable<TooltipState>({ text: '', anchor: null });

export interface MenuItem {
    label: string;
    action: () => void | Promise<void>;
}

export interface MenuState {
    x: number;
    y: number;
    items: MenuItem[];
}

export const menu = writable<MenuState | null>(null);

/** FX Rack panel shown over the faceplate (toolbar button). */
export const fxRackOpen = writable(false);

/** Flash a message in the tooltip at an element (e.g. "Can't paste"). */
export function flashTooltip(node: Element, text: string, ms = 1200): void {
    tooltip.set({ text, anchor: node.getBoundingClientRect() });
    setTimeout(() => tooltip.update((t) => (t.text === text ? { text: '', anchor: null } : t)), ms);
}

// Clipboard: the system clipboard through the plugin (the WebView may block it for the page), the
// browser's in dev mode, and an in-memory copy if both fail.
let localClip = '';

export async function copyText(text: string): Promise<void> {
    localClip = text;
    const nativeCopy = nativeFunction('copyToClipboard');
    try {
        if (nativeCopy) await nativeCopy(text);
        else await navigator.clipboard.writeText(text);
    } catch (err) {
        console.warn('[clipboard] system clipboard unavailable, using local copy', err);
    }
}

export async function pasteText(): Promise<string> {
    const nativePaste = nativeFunction('pasteFromClipboard');
    try {
        const t = nativePaste ? await nativePaste() : await navigator.clipboard.readText();
        if (typeof t === 'string' && t) return t;
    } catch (err) {
        console.warn('[clipboard] system clipboard unavailable, using local copy', err);
    }
    return localClip;
}
