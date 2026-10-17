/** Right-click context menu (spec §10): Set to default, Copy value, Paste value. */

import type { Action } from 'svelte/action';
import { menu as menuStore, copyText, pasteText, flashTooltip, type MenuItem } from '../stores/overlay';

export interface ValueMenuParams {
    /** Restore the default value. */
    reset: () => void;
    /** Displayed value as text. */
    copy: () => string;
    /** Parse and apply pasted text; return false if it was invalid. */
    paste: (text: string) => boolean;
}

export function valueMenuItems(node: Element, p: ValueMenuParams): MenuItem[] {
    return [
        { label: 'Set to default', action: p.reset },
        { label: 'Copy value', action: () => copyText(p.copy()) },
        {
            label: 'Paste value',
            action: async () => {
                if (!p.paste((await pasteText()).trim())) flashTooltip(node, "Can't paste");
            },
        },
    ];
}

export const menu: Action<HTMLElement, ValueMenuParams> = (node, params) => {
    let p = params;
    const open = (e: MouseEvent) => {
        e.preventDefault();
        menuStore.set({ x: e.clientX, y: e.clientY, items: valueMenuItems(node, p) });
    };
    node.addEventListener('contextmenu', open);
    return {
        update(next) {
            p = next;
        },
        destroy() {
            node.removeEventListener('contextmenu', open);
        },
    };
};

/** Parse a pasted toggle value: on/off/1/0/true/false. */
export function parseFlag(text: string): boolean | null {
    const t = text.toLowerCase();
    if (['on', '1', 'true'].includes(t)) return true;
    if (['off', '0', 'false'].includes(t)) return false;
    return null;
}

/** Parse a pasted number, tolerating units like "dB" or "Hz". */
export function parseNumber(text: string): number | null {
    const m = /^[+-]?\d*\.?\d+/.exec(text.replace(',', '.'));
    if (!m) return null;
    const n = Number(m[0]);
    return Number.isFinite(n) ? n : null;
}
