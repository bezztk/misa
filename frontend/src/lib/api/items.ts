import {api} from "./client";
import type {ItemHierarchy} from "../models/item";

export function getItemHierarchy(): Promise<ItemHierarchy[]> {
    return api<ItemHierarchy[]>("/items/hierarchy");
}

export function updateItemParent(id: number, parentId: number | null): Promise<void> {
    return api<void>(`/items/${id}/parent`, {
        method: "PATCH",
        body: JSON.stringify({parent_id: parentId}),
    });
}
