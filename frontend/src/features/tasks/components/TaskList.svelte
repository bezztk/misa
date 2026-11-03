<script lang="ts">
    import {onMount} from "svelte";
    import {
        columnFilteringFeature,
        columnVisibilityFeature,
        createFilteredRowModel,
        createSortedRowModel,
        createTable,
        FlexRender,
        globalFilteringFeature,
        rowSortingFeature,
        tableFeatures,
    } from "@tanstack/svelte-table";
    import {getDevelopmentCategories} from "../../../lib/api/developmentCategories";
    import {deleteTask, getTaskMetadata, getTasks} from "../../../lib/api/tasks";
    import type {DevelopmentCategory} from "../../../lib/models/developmentCategory";
    import type {Task} from "../../../lib/models/task";
    import type {TaskMetadata} from "../../../lib/models/taskMetadata";
    import TaskCreate from "./TaskCreate.svelte";
    import TaskEdit from "./TaskEdit.svelte";

    interface TaskTableRow {
        task: Task;
        name: string;
        description: string;
        status: string;
        category: string;
        group: string;
        completedAt: string;
    }

    const tableFeatureSet = tableFeatures({
        columnFilteringFeature,
        globalFilteringFeature,
        columnVisibilityFeature,
        rowSortingFeature,
        filteredRowModel: createFilteredRowModel(),
        sortedRowModel: createSortedRowModel(),
    });

    const columns = [
        {accessorKey: "name", header: "Name"},
        {accessorKey: "status", header: "Status"},
        {accessorKey: "category", header: "Kategorie"},
        {accessorKey: "group", header: "Gruppe"},
        {accessorKey: "completedAt", header: "Abgeschlossen"},
        {accessorKey: "description", header: "Beschreibung"},
    ];

    let tasks: Task[] = $state([]);
    let metadata: TaskMetadata | null = $state(null);
    let categories: DevelopmentCategory[] = $state([]);
    let selectedView: "all" | "development" | number = $state("all");
    let selectedTask: Task | null = $state(null);
    let editingTask: Task | null = $state(null);
    let createGroupId: number | null = $state(null);
    let creating = $state(false);
    let error = $state("");
    let search = $state("");
    let statusFilter = $state("");
    let createDialog: HTMLDialogElement;
    let editDialog: HTMLDialogElement;

    let visibleTasks = $derived(tasks.filter(task => {
        if (selectedView === "development") return task.group_id === null;
        if (selectedView === "all") return true;
        return task.group_id === selectedView;
    }));

    let tableData = $derived<TaskTableRow[]>(visibleTasks.map(task => ({
        task,
        name: task.name,
        description: task.description ?? "",
        status: metadata?.states.find(state => state.id === task.state_id)?.name ?? "",
        category: categories.find(category => category.item_id === task.parent_id)?.name ?? "",
        group: metadata?.groups.find(group => group.id === task.group_id)?.name ?? "",
        completedAt: task.completed_at
            ? new Date(task.completed_at).toLocaleDateString("de-DE")
            : "",
    })));

    const table = createTable({
        features: tableFeatureSet,
        columns,
        get data() {
            return tableData;
        },
        initialState: {
            columnVisibility: {description: false},
        },
    });

    export async function loadTasks() {
        try {
            error = "";
            tasks = await getTasks();
            if (selectedTask) {
                selectedTask = tasks.find(task => task.item_id === selectedTask?.item_id) ?? null;
            }
        } catch (err) {
            error = String(err);
        }
    }

    onMount(async () => {
        try {
            [metadata, categories] = await Promise.all([
                getTaskMetadata(),
                getDevelopmentCategories(),
            ]);
            await loadTasks();
        } catch (err) {
            error = String(err);
        }
    });

    function openCreate() {
        createGroupId = typeof selectedView === "number"
            ? selectedView
            : metadata?.groups[0]?.id ?? null;
        creating = true;
        createDialog.showModal();
    }

    function openEdit(task: Task) {
        editingTask = task;
        editDialog.showModal();
    }

    async function removeSelected() {
        if (!selectedTask) return;

        await deleteTask(selectedTask.item_id);
        selectedTask = null;
        await loadTasks();
    }
</script>

<div class="task-layout" class:with-detail={selectedTask !== null}>
    <section>
        <div class="heading">
            <h2>Aufgaben</h2>
            <button onclick={openCreate}>Neue Aufgabe</button>
        </div>

        <div class="table-controls">
            <select
                aria-label="Aufgabenansicht"
                bind:value={selectedView}
                onchange={() => selectedTask = null}
            >
                <option value="all">Alle Aufgaben</option>
                <optgroup label="Gruppen">
                    {#each metadata?.groups ?? [] as group}
                        <option value={group.id}>{group.name}</option>
                    {/each}
                </optgroup>
                <option value="development">Entwicklung</option>
            </select>
            <input
                type="search"
                placeholder="Aufgaben durchsuchen"
                bind:value={search}
                oninput={() => table.setGlobalFilter(search)}
            />
            <select
                aria-label="Status filtern"
                bind:value={statusFilter}
                onchange={() => table.getColumn("status")?.setFilterValue(statusFilter || undefined)}
            >
                <option value="">Alle Status</option>
                {#each metadata?.states ?? [] as state}
                    <option value={state.name}>{state.name}</option>
                {/each}
            </select>
            <details class="column-menu">
                <summary>Spalten</summary>
                <div>
                    {#each table.getAllLeafColumns() as column (column.id)}
                        <label>
                            <input
                                type="checkbox"
                                checked={column.getIsVisible()}
                                onchange={(event) => column.toggleVisibility(event.currentTarget.checked)}
                            />
                            {column.columnDef.header}
                        </label>
                    {/each}
                </div>
            </details>
        </div>

        {#if error}
            <p>{error}</p>
        {:else if table.getRowModel().rows.length === 0}
            <p>Keine Aufgaben vorhanden.</p>
        {:else}
            <div class="table-scroll">
                <table>
                    <thead>
                        {#each table.getHeaderGroups() as headerGroup (headerGroup.id)}
                            <tr>
                                {#each headerGroup.headers as header (header.id)}
                                    <th>
                                        {#if !header.isPlaceholder}
                                            <button onclick={() => header.column.toggleSorting()}>
                                                <FlexRender {header} />
                                                {#if header.column.getIsSorted() === "asc"}▲{:else if header.column.getIsSorted() === "desc"}▼{/if}
                                            </button>
                                        {/if}
                                    </th>
                                {/each}
                            </tr>
                        {/each}
                    </thead>
                    <tbody>
                        {#each table.getRowModel().rows as row (row.id)}
                            <tr
                                class:selected={selectedTask?.item_id === row.original.task.item_id}
                                onclick={() => selectedTask = row.original.task}
                            >
                                {#each row.getVisibleCells() as cell (cell.id)}
                                    <td><FlexRender {cell} /></td>
                                {/each}
                            </tr>
                        {/each}
                    </tbody>
                </table>
            </div>
        {/if}
    </section>

    {#if selectedTask}
        <aside class="task-details">
            <button class="close" aria-label="Detailbereich schließen" onclick={() => selectedTask = null}>×</button>
            <h2>{selectedTask.name}</h2>
            <p class="type">Aufgabe</p>

            {#if selectedTask.description}
                <h3>Beschreibung</h3>
                <p>{selectedTask.description}</p>
            {/if}

            <p><strong>Status:</strong> {metadata?.states.find(state => state.id === selectedTask?.state_id)?.name}</p>

            {#if selectedTask.group_id !== null}
                <p><strong>Gruppe:</strong> {metadata?.groups.find(group => group.id === selectedTask?.group_id)?.name}</p>
            {/if}

            {#if selectedTask.parent_id !== null && categories.some(category => category.item_id === selectedTask?.parent_id)}
                <p><strong>Kategorie:</strong> {categories.find(category => category.item_id === selectedTask?.parent_id)?.name}</p>
            {/if}

            {#if selectedTask.completed_at}
                <p><strong>Abgeschlossen am:</strong> {new Date(selectedTask.completed_at).toLocaleDateString("de-DE")}</p>
            {/if}

            <div class="detail-actions">
                <button onclick={() => selectedTask && openEdit(selectedTask)}>Bearbeiten</button>
                <button onclick={() => void removeSelected()}>Löschen</button>
            </div>
        </aside>
    {/if}
</div>

<dialog bind:this={createDialog} onclose={() => creating = false}>
    {#if creating}
        <TaskCreate
            initialGroupId={createGroupId}
            development={selectedView === "development"}
            onCancel={() => createDialog.close()}
            onCreated={() => {
                createDialog.close();
                void loadTasks();
            }}
        />
    {/if}
</dialog>

<dialog bind:this={editDialog} onclose={() => editingTask = null}>
    {#if editingTask}
        <TaskEdit
            task={editingTask}
            onCancel={() => editDialog.close()}
            onUpdated={async () => {
                editDialog.close();
                await loadTasks();
            }}
        />
    {/if}
</dialog>

<style>
    .task-layout {
        display: grid;
        grid-template-columns: minmax(0, 1fr);
        gap: 2rem;
    }

    .task-layout.with-detail {
        grid-template-columns: minmax(18rem, 1fr) 18rem;
    }

    .heading {
        display: flex;
        align-items: center;
        justify-content: space-between;
    }

    .heading h2 {
        margin-top: 0;
    }

    .table-controls {
        display: flex;
        align-items: center;
        gap: 0.5rem;
        margin-bottom: 0.75rem;
        flex-wrap: wrap;
    }

    .table-controls input,
    .table-controls select,
    .column-menu summary {
        padding: 0.35rem 0.5rem;
    }

    .column-menu {
        position: relative;
    }

    .column-menu summary {
        border: 1px solid #cccccc;
        border-radius: 0.2rem;
        cursor: pointer;
    }

    .column-menu > div {
        position: absolute;
        z-index: 2;
        top: 2rem;
        right: 0;
        display: flex;
        min-width: 10rem;
        padding: 0.5rem;
        border: 1px solid #cccccc;
        background: white;
        flex-direction: column;
        gap: 0.35rem;
    }

    .table-scroll {
        overflow-x: auto;
    }

    table {
        width: 100%;
        border-collapse: collapse;
        font-size: 0.9rem;
    }

    th,
    td {
        padding: 0.45rem 0.6rem;
        border-bottom: 1px solid #dddddd;
        text-align: left;
        white-space: nowrap;
    }

    th button {
        padding: 0;
        border: 0;
        background: transparent;
        font: inherit;
        font-weight: bold;
        cursor: pointer;
    }

    tbody tr {
        cursor: pointer;
    }

    tbody tr:hover,
    tbody tr.selected {
        background: #eeeeee;
    }

    .task-details {
        position: sticky;
        top: 2rem;
        align-self: start;
        padding-left: 1.5rem;
        border-left: 1px solid #dddddd;
    }

    .task-details h2 {
        margin: 0 2rem 0.25rem 0;
    }

    .task-details h3 {
        margin-bottom: 0.25rem;
        font-size: 1rem;
    }

    .task-details p {
        margin: 0.5rem 0;
    }

    .type {
        color: #666666;
    }

    .close {
        float: right;
        border: 0;
        background: transparent;
        font-size: 1.25rem;
        cursor: pointer;
    }

    .detail-actions {
        display: flex;
        gap: 0.5rem;
        margin-top: 1.25rem;
    }
</style>
