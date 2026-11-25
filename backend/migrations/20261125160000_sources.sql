-- Zotero itemTypes und deutsche Anzeigenamen aus dem Translation-Server-Schema.
CREATE TABLE resource_types
(
    id           smallint PRIMARY KEY,
    name         text NOT NULL UNIQUE,
    display_name text NOT NULL
);
INSERT INTO resource_types (id, name, display_name)
VALUES (1, 'annotation', 'Anmerkung'),
       (2, 'artwork', 'Kunstwerk'),
       (3, 'attachment', 'Anhang'),
       (4, 'audioRecording', 'Tonaufnahme'),
       (5, 'bill', 'Gesetzentwurf'),
       (6, 'blogPost', 'Blog-Post'),
       (7, 'book', 'Buch'),
       (8, 'bookSection', 'Buchteil'),
       (9, 'case', 'Gerichtsentscheidung'),
       (10, 'computerProgram', 'Software'),
       (11, 'conferencePaper', 'Konferenz-Paper'),
       (12, 'dataset', 'Datensatz'),
       (13, 'dictionaryEntry', 'Wörterbucheintrag'),
       (14, 'document', 'Dokument'),
       (15, 'email', 'E-Mail'),
       (16, 'encyclopediaArticle', 'Enzyklopädieartikel'),
       (17, 'film', 'Film'),
       (18, 'forumPost', 'Foren-Eintrag'),
       (19, 'hearing', 'Anhörung'),
       (20, 'instantMessage', 'Instant-Message'),
       (21, 'interview', 'Interview'),
       (22, 'journalArticle', 'Zeitschriftenartikel'),
       (23, 'letter', 'Brief'),
       (24, 'magazineArticle', 'Magazin-Artikel'),
       (25, 'manuscript', 'Manuskript'),
       (26, 'map', 'Karte'),
       (27, 'newspaperArticle', 'Zeitungsartikel'),
       (28, 'note', 'Notiz'),
       (29, 'patent', 'Patent'),
       (30, 'podcast', 'Podcast'),
       (31, 'preprint', 'Preprint'),
       (32, 'presentation', 'Vortrag'),
       (33, 'radioBroadcast', 'Radiosendung'),
       (34, 'report', 'Bericht'),
       (35, 'standard', 'Norm'),
       (36, 'statute', 'Gesetz'),
       (37, 'thesis', 'Dissertation'),
       (38, 'tvBroadcast', 'Fernsehsendung'),
       (39, 'videoRecording', 'Videoaufnahme'),
       (40, 'webpage', 'Webseite');

CREATE TABLE sources
(
    id               uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    resource_type_id smallint NOT NULL REFERENCES resource_types (id),
    title            text,
    url              text NOT NULL,
    description      text,
    thumbnail_url    text,
    metadata         jsonb NOT NULL,
    created_at       timestamptz NOT NULL DEFAULT now(),
    updated_at       timestamptz NOT NULL DEFAULT now()
);
