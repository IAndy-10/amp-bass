/** Right-click context menu: Set to default, Copy value, Paste value. */

import type { Action } from 'svelte/action';
import { menu as menuStore, copyText, pasteText, flashTooltip, type MenuItem } from '../stores/overlay';
import { nativeFunction } from '../../bridge/bridge';

export interface ValueMenuParams {
    /** Parameter ID: a VST3 host adds its own items for it (e.g. Ableton's Show Automation). */
    param: string;
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
        const items = valueMenuItems(node, p);
        const showOwn = () => menuStore.set({ x: e.clientX, y: e.clientY, items });
        // The plugin shows one native menu with the host's items and ours, and answers with the index of
        // ours that was picked (-1 = none), or false when the host has no menu for parameters.
        const showWithHost = nativeFunction('showParameterMenu');
        if (!showWithHost) return showOwn();
        showWithHost(p.param, items.map((i) => i.label))
            .then((picked) => {
                if (picked === false) showOwn();
                else if (typeof picked === 'number' && picked >= 0) void items[picked]?.action();
            })
            .catch((err) => {
                console.warn('[menu] host parameter menu failed, showing the plugin menu', err);
                showOwn();
            });
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
