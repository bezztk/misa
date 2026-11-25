ALTER TABLE media
    ADD COLUMN tmdb_vote_average double precision CHECK (tmdb_vote_average BETWEEN 0 AND 10),
    ADD COLUMN tmdb_vote_count integer CHECK (tmdb_vote_count >= 0);
