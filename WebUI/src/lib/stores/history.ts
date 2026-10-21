/**
 * JS-only undo/redo: one snapshot per completed gesture, capped at 100.
 * Applying a snapshot writes through the bridge, so the DSP follows.
 */

import { writable, derived } from 'svelte/store';
import { onGestureCommitted, applySnapshot, takeSnapshot, type Snapshot } from './amp';

const LIMIT = 100;

const undoStack = writable<Snapshot[]>([]);
const redoStack = writable<Snapshot[]>([]);

export const canUndo = derived(undoStack, (s) => s.length > 0);
export const canRedo = derived(redoStack, (s) => s.length > 0);

let applying = false;

onGestureCommitted((before) => {
    if (applying) return;
    undoStack.update((s) => [...s, before].slice(-LIMIT));
    redoStack.set([]);
});

function step(from: typeof undoStack, to: typeof undoStack): void {
    let target: Snapshot | undefined;
    from.update((s) => {
        target = s[s.length - 1];
        return s.slice(0, -1);
    });
    if (!target) return;
    to.update((s) => [...s, takeSnapshot()].slice(-LIMIT));
    applying = true;
    try {
        applySnapshot(target);
    } finally {
        applying = false;
    }
}

export const undo = (): void => step(undoStack, redoStack);
export const redo = (): void => step(redoStack, undoStack);
