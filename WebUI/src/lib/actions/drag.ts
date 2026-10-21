/**
 * Vertical pointer drag with pointer capture.
 *
 * Reports the accumulated vertical distance in screen px (up = positive),
 * with Shift making movement 10× finer. Components combine it with the value
 * captured at gesture start, which avoids drift from repeated snapping.
 */

import type { Action } from 'svelte/action';

export interface DragParams {
    onStart: () => void;
    onDrag: (dyUp: number) => void;
    onEnd: () => void;
    disabled?: boolean;
}

const FINE_FACTOR = 0.1;

export const drag: Action<HTMLElement, DragParams> = (node, params) => {
    let p = params;
    let pointerId: number | null = null;
    let lastY = 0;
    let total = 0;

    const down = (e: PointerEvent) => {
        if (p.disabled || e.button !== 0) return;
        e.preventDefault();
        node.focus();
        pointerId = e.pointerId;
        node.setPointerCapture(e.pointerId);
        lastY = e.clientY;
        total = 0;
        p.onStart();
    };

    const move = (e: PointerEvent) => {
        if (e.pointerId !== pointerId) return;
        const dy = lastY - e.clientY;
        lastY = e.clientY;
        total += e.shiftKey ? dy * FINE_FACTOR : dy;
        p.onDrag(total);
    };

    const up = (e: PointerEvent) => {
        if (e.pointerId !== pointerId) return;
        pointerId = null;
        if (node.hasPointerCapture(e.pointerId)) node.releasePointerCapture(e.pointerId);
        p.onEnd();
    };

    node.addEventListener('pointerdown', down);
    node.addEventListener('pointermove', move);
    node.addEventListener('pointerup', up);
    node.addEventListener('pointercancel', up);

    return {
        update(next) {
            p = next;
        },
        destroy() {
            node.removeEventListener('pointerdown', down);
            node.removeEventListener('pointermove', move);
            node.removeEventListener('pointerup', up);
            node.removeEventListener('pointercancel', up);
        },
    };
};

/** Non-passive wheel listener, so the page doesn't scroll while a control
 *  consumes the wheel. Calls `handler(+1 | -1, shift)` per notch. */
export const wheel: Action<HTMLElement, (dir: 1 | -1, shift: boolean) => void> = (node, handler) => {
    let h = handler;
    const onWheel = (e: WheelEvent) => {
        const d = e.deltaY !== 0 ? e.deltaY : e.deltaX;
        if (d === 0) return;
        e.preventDefault();
        h(d < 0 ? 1 : -1, e.shiftKey);
    };
    node.addEventListener('wheel', onWheel, { passive: false });
    return {
        update(next) {
            h = next;
        },
        destroy() {
            node.removeEventListener('wheel', onWheel);
        },
    };
};
