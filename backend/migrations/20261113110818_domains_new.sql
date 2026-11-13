-- Add migration script here
alter table domains drop column display_name;
delete from domains;
INSERT INTO domains (name)
VALUES
    ('Mathematik'),
    ('Informatik und Informationswissenschaft'),
    ('Physik'),
    ('Chemie'),
    ('Biologie'),
    ('Astronomie und Astrophysik'),
    ('Geowissenschaften'),
    ('Umweltwissenschaften'),
    ('Medizin und Gesundheitswissenschaften'),
    ('Technologie und Ingenieurwesen'),
    ('Psychologie'),
    ('Sozialwissenschaften'),
    ('Wirtschaftswissenschaften'),
    ('Philosophie'),
    ('Religion'),
    ('Geografie'),
    ('Kunst'),
    ('Sport und Bewegung'),
    ('Geschichte'),
    ('Sprache und Kommunikation');