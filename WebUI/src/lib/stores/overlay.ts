/** Shared tooltip and context-menu state (spec §9 Tooltip / ContextMenu, §10). */

import { writable } from 'svelte/store';

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

/** Flash a message in the tooltip at an element (e.g. "Can't paste"). */
export function flashTooltip(node: Element, text: string, ms = 1200): void {
    tooltip.set({ text, anchor: node.getBoundingClientRect() });
    setTimeout(() => tooltip.update((t) => (t.text === text ? { text: '', anchor: null } : t)), ms);
}

// Clipboard: try the system clipboard, keep an in-memory copy for WebViews that block it.
let localClip = '';

export async function copyText(text: string): Promise<void> {
    localClip = text;
    try {
        await navigator.clipboard.writeText(text);
    } catch (err) {
        console.warn('[clipboard] system clipboard unavailable, using local copy', err);
    }
}

export async function pasteText(): Promise<string> {
    try {
        const t = await navigator.clipboard.readText();
        if (t) return t;
    } catch (err) {
        console.warn('[clipboard] system clipboard unavailable, using local copy', err);
    }
    return localClip;
}
