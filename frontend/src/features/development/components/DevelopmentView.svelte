<script lang="ts">
    import {onMount} from "svelte";
    import {
        createDevelopmentArea,
        deleteDevelopmentArea,
        getDevelopmentAreas,
        updateDevelopmentArea,
    } from "../../../lib/api/developmentAreas";
    import type {DevelopmentArea} from "../../../lib/models/developmentArea";
    import DevelopmentTree from "./DevelopmentTree.svelte";

    let areas: DevelopmentArea[] = $state([]);
    let expandedAreaIds: number[] = $state([]);
    let selectedAreaId: number | null = $state(null);
    let selectedItemId: number | null = $state(null);
    let editingArea: DevelopmentArea | null = $state(null);
    let name = $state("");
    let description = $state("");
    let error = $state("");
    let formError = $state("");
    let pending = $state(false);
    let dialog: HTMLDialogElement;

    async function loadAreas() {
        try {
            error = "";
            areas = await getDevelopmentAreas();
        } catch (err) {
            error = String(err);
        }
    }

    function toggleArea(id: number) {
        expandedAreaIds = expandedAreaIds.includes(id)
            ? expandedAreaIds.filter(areaId => areaId !== id)
            : [...expandedAreaIds, id];
    }

    function openCreate() {
        error = "";
        formError = "";
        editingArea = null;
        name = "";
        description = "";
        dialog.showModal();
    }

    function openEdit(area: DevelopmentArea) {
        error = "";
        formError = "";
        editingArea = area;
        name = area.name;
        description = area.description ?? "";
        dialog.showModal();
    }

    async function submit() {
        if (pending) return;

        try {
            formError = "";
            pending = true;

            if (editingArea) {
                await updateDevelopmentArea(editingArea.id, {name, description});
            } else {
                await createDevelopmentArea({name, description});
            }

            dialog.close();
            await loadAreas();
        } catch (err) {
            formError = String(err);
        } finally {
            pending = false;
        }
    }

    async function remove(area: DevelopmentArea) {
        if (pending || !confirm(`Area "${area.name}" löschen?`)) return;

        try {
            error = "";
            pending = true;
            await deleteDevelopmentArea(area.id);
            await loadAreas();
        } catch (err) {
            error = String(err);
        } finally {
            pending = false;
        }
    }

    onMount(() => {
        void loadAreas();
    });
</script>

<section>
    <h2>Entwicklung</h2>
    <button onclick={openCreate} disabled={pending}>Neue Area</button>

    {#if error}
        <p>{error}</p>
    {/if}

    {#each areas as area (area.id)}
        <article>
            <div class="area-heading">
                <button
                        class="arrow"
                        aria-label={expandedAreaIds.includes(area.id) ? "Area einklappen" : "Area ausklappen"}
                        onclick={() => toggleArea(area.id)}
                >
                    {expandedAreaIds.includes(area.id) ? "▼" : "▶"}
                </button>
                <span aria-hidden="true">▰</span>
                <button
                        class="area-name"
                        class:selected={selectedAreaId === area.id}
                        onclick={() => {
                            selectedAreaId = area.id;
                            selectedItemId = null;
                        }}
                >
                    {area.name}
                </button>
                <button onclick={() => openEdit(area)} disabled={pending}>Bearbeiten</button>
                <button onclick={() => remove(area)} disabled={pending}>Löschen</button>
            </div>

            {#if area.description}
                <p>{area.description}</p>
            {/if}

            {#if expandedAreaIds.includes(area.id)}
                <DevelopmentTree
                        areaId={area.id}
                        {selectedItemId}
                        onSelect={(id) => {
                            selectedItemId = id;
                            selectedAreaId = null;
                        }}
                />
            {/if}
        </article>
    {/each}
</section>

<dialog bind:this={dialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void submit();
    }}>
        <h3>{editingArea ? "Area bearbeiten" : "Neue Area"}</h3>

        <label for="area-name">Name</label>
        <input id="area-name" bind:value={name} required />

        <label for="area-description">Beschreibung</label>
        <textarea id="area-description" bind:value={description}></textarea>

        {#if formError}
            <p>{formError}</p>
        {/if}

        <button type="submit" disabled={pending}>
            {pending ? "Wird gespeichert..." : "Speichern"}
        </button>
        <button type="button" onclick={() => dialog.close()}>Abbrechen</button>
    </form>
</dialog>

<style>
    article {
        padding: 1rem 0;
        border-bottom: 1px solid #ddd;
    }

    .area-heading {
        display: flex;
        align-items: center;
        gap: 0.5rem;
    }

    .arrow {
        width: 1.75rem;
        border: 0;
        background: transparent;
        cursor: pointer;
    }

    .area-name {
        min-width: 10rem;
        padding: 0.35rem 0.5rem;
        border: 0;
        border-radius: 0.25rem;
        background: transparent;
        text-align: left;
        font-weight: bold;
        cursor: pointer;
    }

    .area-name.selected {
        background: #e8e8e8;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
