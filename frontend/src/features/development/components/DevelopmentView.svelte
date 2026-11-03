<script lang="ts">
    import {onMount} from "svelte";
    import {
        createDevelopmentArea,
        deleteDevelopmentArea,
        getDevelopmentAreas,
        updateDevelopmentArea,
    } from "../../../lib/api/developmentAreas";
    import type {DevelopmentArea} from "../../../lib/models/developmentArea";
    import DevelopmentCategoryList from "./DevelopmentCategoryList.svelte";
    import SkillList from "./SkillList.svelte";

    let areas: DevelopmentArea[] = $state([]);
    let selectedArea: DevelopmentArea | null = $state(null);
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

            if (selectedArea) {
                selectedArea = areas.find(area => area.id === selectedArea?.id) ?? null;
            }
        } catch (err) {
            error = String(err);
        }
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

    {#if selectedArea}
        <p>Ausgewählt: <strong>{selectedArea.name}</strong></p>
    {/if}

    {#if error}
        <p>{error}</p>
    {/if}

    {#each areas as area (area.id)}
        <article>
            <button onclick={() => selectedArea = area}>{area.name}</button>
            <button onclick={() => openEdit(area)} disabled={pending}>Bearbeiten</button>
            <button onclick={() => remove(area)} disabled={pending}>Löschen</button>

            {#if area.description}
                <p>{area.description}</p>
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

{#if selectedArea}
    {#key selectedArea.id}
        <DevelopmentCategoryList areaId={selectedArea.id} />
        <SkillList areaId={selectedArea.id} />
    {/key}
{/if}

<style>
    article {
        padding: 1rem 0;
        border-bottom: 1px solid #ddd;
    }

    article button {
        margin-right: 0.5rem;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
