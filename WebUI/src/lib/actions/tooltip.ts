/**
 * Hover tooltip: shows after 350 ms of hover, immediately while `active`
 * (dragging), and follows `text` updates.
 */

import type { Action } from 'svelte/action';
import { tooltip as tooltipStore } from '../stores/overlay';

export interface TooltipParams {
    text: string;
    active?: boolean;
}

const HOVER_DELAY_MS = 350;

export const tooltip: Action<HTMLElement, TooltipParams> = (node, params) => {
    let p = params;
    let hovered = false;
    let shown = false;
    let timer: ReturnType<typeof setTimeout> | undefined;

    const show = () => {
        shown = true;
        tooltipStore.set({ text: p.text, anchor: node.getBoundingClientRect() });
    };
    const hide = () => {
        shown = false;
        tooltipStore.set({ text: '', anchor: null });
    };

    const enter = () => {
        hovered = true;
        clearTimeout(timer);
        timer = setTimeout(show, HOVER_DELAY_MS);
    };
    const leave = () => {
        hovered = false;
        clearTimeout(timer);
        if (!p.active) hide();
    };

    node.addEventListener('pointerenter', enter);
    node.addEventListener('pointerleave', leave);
    node.addEventListener('focus', show);
    node.addEventListener('blur', hide);

    return {
        update(next) {
            const wasActive = p.active;
            p = next;
            if (p.active || (shown && hovered) || (shown && document.activeElement === node)) show();
            else if (wasActive && !hovered) hide();
        },
        destroy() {
            clearTimeout(timer);
            node.removeEventListener('pointerenter', enter);
            node.removeEventListener('pointerleave', leave);
            node.removeEventListener('focus', show);
            node.removeEventListener('blur', hide);
            if (shown) hide();
        },
    };
};
