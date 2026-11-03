<script lang="ts">
    import {onMount} from "svelte";
    import {
        createDevelopmentCategory,
        deleteDevelopmentCategory,
        getDevelopmentCategories,
        updateDevelopmentCategory,
    } from "../../../lib/api/developmentCategories";
    import {createGoal, deleteGoal, getGoals, updateGoal} from "../../../lib/api/goals";
    import {getAllowedTypeRelations, getItemHierarchy, updateItemParent} from "../../../lib/api/items";
    import {
        createSkill,
        deleteSkill,
        getSkillMetadata,
        getSkills,
        updateSkill,
    } from "../../../lib/api/skills";
    import {createTask, deleteTask, getTaskMetadata, getTasks} from "../../../lib/api/tasks";
    import type {AllowedTypeRelation} from "../../../lib/models/item";
    import type {SkillMetadata} from "../../../lib/models/skill";
    import type {Task} from "../../../lib/models/task";
    import TaskEdit from "../../tasks/components/TaskEdit.svelte";

    let {areaId, selectedItemId, onSelect, onDeleted}: {
        areaId: number;
        selectedItemId: number | null;
        onSelect: (item: TreeItem, edit: () => void, remove: () => void, editParent: () => void) => void;
        onDeleted: (id: number) => void;
    } = $props();

    type ItemType = "Category" | "Skill" | "Goal" | "Task";
    type EditableType = "Category" | "Skill" | "Goal" | "Task";

    interface TreeItem {
        id: number;
        name: string;
        type: ItemType;
        parentId: number | null;
        description: string | null;
        stateId: number | null;
        stateName: string | null;
        groupName: string | null;
        isCompleted: boolean;
        completedAt: string | null;
        dueAt: string | null;
        parentName: string | null;
        task: Task | null;
    }

    interface TreeRow extends TreeItem {
        depth: number;
        hasChildren: boolean;
    }

    const symbols: Record<ItemType, string> = {
        Category: "▣",
        Skill: "◆",
        Goal: "◎",
        Task: "✓",
    };

    let items: TreeItem[] = $state([]);
    let rows: TreeRow[] = $state([]);
    let collapsedItemIds: number[] = $state([]);
    let allowedTypeRelations: AllowedTypeRelation[] = $state([]);
    let skillMetadata: SkillMetadata | null = $state(null);
    let error = $state("");
    let pending = $state(false);

    let itemDialog: HTMLDialogElement;
    let editingItem: TreeItem | null = $state(null);
    let creatingParent: TreeItem | null = $state(null);
    let formType: EditableType = $state("Category");
    let name = $state("");
    let description = $state("");
    let stateId: number | null = $state(null);
    let isCompleted = $state(false);
    let dueDate = $state("");
    let itemFormError = $state("");

    let parentDialog: HTMLDialogElement;
    let parentItem: TreeItem | null = $state(null);
    let parentId: number | null = $state(null);
    let parentFormError = $state("");
    let taskDialog: HTMLDialogElement;
    let editingTask: Task | null = $state(null);

    function selectItem(item: TreeItem) {
        onSelect(
            item,
            () => openEdit(item),
            () => void removeItem(item),
            () => openParentDialog(item),
        );
    }

    function buildRows() {
        const children = new Map<number | null, TreeItem[]>();

        for (const item of items) {
            const siblings = children.get(item.parentId) ?? [];
            siblings.push(item);
            children.set(item.parentId, siblings);
        }

        const nextRows: TreeRow[] = [];

        function addChildren(parentId: number | null, depth: number) {
            for (const item of children.get(parentId) ?? []) {
                const hasChildren = children.has(item.id);
                nextRows.push({...item, depth, hasChildren});

                if (!collapsedItemIds.includes(item.id)) {
                    addChildren(item.id, depth + 1);
                }
            }
        }

        addChildren(null, 0);
        rows = nextRows;
    }

    function toggleItem(id: number) {
        collapsedItemIds = collapsedItemIds.includes(id)
            ? collapsedItemIds.filter(itemId => itemId !== id)
            : [...collapsedItemIds, id];
        buildRows();
    }

    function allowedChildTypes(parentType: ItemType): EditableType[] {
        return allowedTypeRelations
            .filter(relation => relation.parent_type === parentType)
            .map(relation => relation.child_type as EditableType);
    }

    async function loadTree() {
        try {
            error = "";

            const [categories, skills, goals, tasks, hierarchy, metadata, taskMetadata, relations] = await Promise.all([
                getDevelopmentCategories(),
                getSkills(),
                getGoals(),
                getTasks(),
                getItemHierarchy(),
                getSkillMetadata(),
                getTaskMetadata(),
                getAllowedTypeRelations(),
            ]);

            skillMetadata = metadata;
            allowedTypeRelations = relations;

            const nextItems: TreeItem[] = [
                ...categories
                    .filter(category => category.area_id === areaId)
                    .map(category => ({
                        id: category.item_id,
                        name: category.name,
                        type: "Category" as const,
                        parentId: null,
                        description: null,
                        stateId: null,
                        stateName: null,
                        groupName: null,
                        isCompleted: false,
                        completedAt: null,
                        dueAt: null,
                        parentName: null,
                        task: null,
                    })),
                ...skills
                    .filter(skill => skill.area_id === areaId)
                    .map(skill => ({
                        id: skill.item_id,
                        name: skill.name,
                        type: "Skill" as const,
                        parentId: null,
                        description: skill.description,
                        stateId: skill.state_id,
                        stateName: metadata.states.find(state => state.id === skill.state_id)?.name ?? null,
                        groupName: null,
                        isCompleted: false,
                        completedAt: null,
                        dueAt: null,
                        parentName: null,
                        task: null,
                    })),
                ...goals
                    .filter(goal => goal.area_id === areaId)
                    .map(goal => ({
                        id: goal.item_id,
                        name: goal.name,
                        type: "Goal" as const,
                        parentId: null,
                        description: goal.description,
                        stateId: null,
                        stateName: null,
                        groupName: null,
                        isCompleted: goal.is_completed,
                        completedAt: goal.completed_at,
                        dueAt: goal.due_at,
                        parentName: null,
                        task: null,
                    })),
                ...tasks
                    .filter(task => task.area_id === areaId)
                    .map(task => ({
                        id: task.item_id,
                        name: task.name,
                        type: "Task" as const,
                        parentId: null,
                        description: task.description,
                        stateId: task.state_id,
                        stateName: taskMetadata.states.find(state => state.id === task.state_id)?.name ?? null,
                        groupName: taskMetadata.groups.find(group => group.id === task.group_id)?.name ?? null,
                        isCompleted: false,
                        completedAt: task.completed_at,
                        dueAt: null,
                        parentName: null,
                        task,
                    })),
            ];

            const itemIds = new Set(nextItems.map(item => item.id));
            const parents = new Map(hierarchy.map(item => [item.child_id, item.parent_id]));

            for (const item of nextItems) {
                const nextParentId = parents.get(item.id);
                item.parentId = nextParentId !== undefined && itemIds.has(nextParentId)
                    ? nextParentId
                    : null;
                item.parentName = nextItems.find(parent => parent.id === item.parentId)?.name ?? null;
            }

            nextItems.sort((a, b) => a.name.localeCompare(b.name));
            items = nextItems;
            collapsedItemIds = collapsedItemIds.filter(id => itemIds.has(id));
            buildRows();

            const selectedItem = nextItems.find(item => item.id === selectedItemId);
            if (selectedItem) selectItem(selectedItem);
        } catch (err) {
            error = String(err);
        }
    }

    function openCreate(type: EditableType, parent: TreeItem) {
        itemFormError = "";
        editingItem = null;
        creatingParent = parent;
        formType = type;
        name = "";
        description = "";
        stateId = skillMetadata?.states[0]?.id ?? null;
        isCompleted = false;
        dueDate = "";
        itemDialog.showModal();
    }

    function openEdit(item: TreeItem) {
        if (item.type === "Task") {
            editingTask = item.task;
            taskDialog.showModal();
            return;
        }

        itemFormError = "";
        editingItem = item;
        creatingParent = null;
        formType = item.type;
        name = item.name;
        description = item.description ?? "";
        stateId = item.stateId;
        isCompleted = item.isCompleted;
        dueDate = item.dueAt?.slice(0, 10) ?? "";
        itemDialog.showModal();
    }

    async function saveItem() {
        if (pending) return;

        try {
            itemFormError = "";
            pending = true;

            if (editingItem) {
                if (formType === "Category") {
                    await updateDevelopmentCategory(editingItem.id, {name});
                } else if (formType === "Skill") {
                    await updateSkill(editingItem.id, {name, description, state_id: stateId});
                } else {
                    const dueAt = dueDate
                        ? new Date(`${dueDate}T00:00:00.000Z`).toISOString()
                        : null;
                    await updateGoal(editingItem.id, {
                        name,
                        description,
                        is_completed: isCompleted,
                        due_at: dueAt ?? undefined,
                        clear_due_at: dueAt === null,
                    });
                }
            } else {
                if (!creatingParent) return;

                let created: {id: number};

                if (formType === "Category") {
                    created = await createDevelopmentCategory({area_id: areaId, name});
                } else if (formType === "Skill") {
                    created = await createSkill({area_id: areaId, name, description, state_id: stateId});
                } else if (formType === "Goal") {
                    const dueAt = dueDate
                        ? new Date(`${dueDate}T00:00:00.000Z`).toISOString()
                        : null;
                    created = await createGoal({area_id: areaId, name, description, due_at: dueAt});
                } else {
                    created = await createTask({area_id: areaId, group_id: null, name, description});
                }

                await updateItemParent(created.id, creatingParent.id);
                collapsedItemIds = collapsedItemIds.filter(id => id !== creatingParent?.id);
            }

            itemDialog.close();
            await loadTree();
        } catch (err) {
            itemFormError = String(err);
        } finally {
            pending = false;
        }
    }

    async function removeItem(item: TreeItem) {
        if (pending || !confirm(`${item.type} "${item.name}" löschen?`)) {
            return;
        }

        try {
            error = "";
            pending = true;

            if (item.type === "Category") {
                await deleteDevelopmentCategory(item.id);
            } else if (item.type === "Skill") {
                await deleteSkill(item.id);
            } else if (item.type === "Goal") {
                await deleteGoal(item.id);
            } else {
                await deleteTask(item.id);
            }

            onDeleted(item.id);
            await loadTree();
        } catch (err) {
            error = String(err);
        } finally {
            pending = false;
        }
    }

    function openParentDialog(item: TreeItem) {
        parentFormError = "";
        parentItem = item;
        parentId = item.parentId;
        parentDialog.showModal();
    }

    async function saveParent() {
        if (!parentItem || pending) return;

        try {
            parentFormError = "";
            pending = true;
            await updateItemParent(parentItem.id, parentId);
            parentDialog.close();
            await loadTree();
        } catch (err) {
            parentFormError = String(err);
        } finally {
            pending = false;
        }
    }

    onMount(() => {
        void loadTree();
    });
</script>

<section>
    {#if error}
        <p>{error}</p>
    {:else if rows.length === 0}
        <p>Keine Elemente vorhanden.</p>
    {:else}
        {#each rows as row (row.id)}
            <div
                class="tree-row"
                class:selected={selectedItemId === row.id}
                style={`padding-left: ${row.depth * 1.5}rem`}
            >
                <span class="toggle-slot">
                    {#if row.hasChildren}
                        <button
                            class="arrow"
                            aria-label={collapsedItemIds.includes(row.id) ? "Öffnen" : "Schließen"}
                            onclick={() => toggleItem(row.id)}
                        >
                            {collapsedItemIds.includes(row.id) ? "▶" : "▼"}
                        </button>
                    {/if}
                </span>
                <span class="type-icon" aria-hidden="true">{symbols[row.type]}</span>
                <button class="item-name" onclick={() => selectItem(row)}>{row.name}</button>
                <span class="item-type">{row.type}</span>
                {#if allowedChildTypes(row.type).length > 0}
                    <details class="create-child">
                        <summary aria-label={`Unterelement für ${row.name} erstellen`}>+</summary>
                        <div class="child-menu">
                        {#each allowedChildTypes(row.type) as type}
                            <button
                                disabled={pending}
                                onclick={(event) => {
                                    event.currentTarget.closest("details")?.removeAttribute("open");
                                    openCreate(type, row);
                                }}
                            >
                                {type}
                            </button>
                        {/each}
                        </div>
                    </details>
                {/if}
            </div>
        {/each}
    {/if}
</section>

<dialog bind:this={itemDialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void saveItem();
    }}>
        <h3>{editingItem ? `${formType} bearbeiten` : `${formType} erstellen`}</h3>

        <label for="item-name">Name</label>
        <input id="item-name" bind:value={name} required />

        {#if formType === "Skill"}
            <label for="skill-state">Status</label>
            <select id="skill-state" bind:value={stateId} required>
                {#each skillMetadata?.states ?? [] as state}
                    <option value={state.id}>{state.name}</option>
                {/each}
            </select>
        {/if}

        {#if formType === "Skill" || formType === "Goal" || formType === "Task"}
            <label for="item-description">Beschreibung</label>
            <textarea id="item-description" bind:value={description}></textarea>
        {/if}

        {#if formType === "Goal"}
            <label for="goal-due-date">Fälligkeitsdatum</label>
            <input id="goal-due-date" type="date" bind:value={dueDate} />

            {#if editingItem}
                <label>
                    <input type="checkbox" bind:checked={isCompleted} />
                    Abgeschlossen
                </label>
            {/if}
        {/if}

        {#if itemFormError}
            <p>{itemFormError}</p>
        {/if}

        <button type="submit" disabled={pending}>
            {pending ? "Wird gespeichert..." : "Speichern"}
        </button>
        <button type="button" onclick={() => itemDialog.close()}>Abbrechen</button>
    </form>
</dialog>

<dialog bind:this={parentDialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void saveParent();
    }}>
        <h3>Parent für {parentItem?.name}</h3>

        <label for="parent">Parent</label>
        <select id="parent" bind:value={parentId}>
            <option value={null}>Kein Parent</option>
            {#each items.filter(item => item.id !== parentItem?.id) as item (item.id)}
                <option value={item.id}>{item.name} ({item.type})</option>
            {/each}
        </select>

        {#if parentFormError}
            <p>{parentFormError}</p>
        {/if}

        <button type="submit" disabled={pending}>
            {pending ? "Wird gespeichert..." : "Speichern"}
        </button>
        <button type="button" onclick={() => parentDialog.close()}>Abbrechen</button>
    </form>
</dialog>

<dialog bind:this={taskDialog} onclose={() => editingTask = null}>
    {#if editingTask}
        <TaskEdit
            task={editingTask}
            onCancel={() => taskDialog.close()}
            onUpdated={async () => {
                taskDialog.close();
                await loadTree();
            }}
        />
    {/if}
</dialog>

<style>
    .tree-row {
        display: flex;
        align-items: center;
        min-height: 2rem;
        margin: 0.125rem 0;
        border-radius: 0.25rem;
        font-size: 0.95rem;
    }

    .tree-row.selected {
        background: #eeeeee;
    }

    .toggle-slot {
        display: inline-flex;
        flex: 0 0 1.75rem;
        width: 1.75rem;
    }

    .arrow {
        width: 1.75rem;
        padding: 0;
        border: 0;
        background: transparent;
        cursor: pointer;
    }

    .type-icon {
        flex: 0 0 1.5rem;
        width: 1.5rem;
        text-align: center;
    }

    .item-name {
        padding: 0.25rem 0.4rem;
        border: 0;
        background: transparent;
        font: inherit;
        text-align: left;
        cursor: pointer;
    }

    .item-type {
        margin-left: 0.25rem;
        color: #666666;
        font-size: 0.85rem;
    }

    .create-child {
        position: relative;
        margin-left: 0.5rem;
    }

    .create-child summary {
        display: grid;
        width: 1.25rem;
        height: 1.25rem;
        padding: 0;
        border: 1px solid #cccccc;
        border-radius: 0.2rem;
        cursor: pointer;
        place-items: center;
        list-style: none;
        line-height: 1;
    }

    .create-child summary::-webkit-details-marker {
        display: none;
    }

    .child-menu {
        position: absolute;
        z-index: 1;
        top: 1.5rem;
        left: 0;
        display: flex;
        min-width: 7rem;
        padding: 0.25rem;
        border: 1px solid #cccccc;
        background: white;
        flex-direction: column;
        gap: 0.125rem;
    }

    .child-menu button {
        padding: 0.25rem 0.5rem;
        border: 0;
        background: transparent;
        text-align: left;
        cursor: pointer;
    }

    .child-menu button:hover {
        background: #eeeeee;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
