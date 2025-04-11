set schema 'tchatator';

set
    plpgsql.extra_errors to 'all';

create function _insert_msg (p_id_compte_sender int, p_id_compte_recipient int, p_content varchar) returns int as $$
with msg_id as (insert into
    tchatator._msg (id_compte_sender, id_compte_recipient, content)
values
    -- condisder 0 as the admin user id.
    (nullif(p_id_compte_sender, 0), p_id_compte_recipient, p_content)
returning
    msg_id
) table msg_id
$$ language sql strict;

create function send_msg (p_id_compte_sender int, p_id_compte_recipient int, p_content varchar) returns int as $$
select
    case
        when (
            -- check is not blocked globally
            select
                full_block_expires_at < localtimestamp
            from
                _member
            where
                id = p_id_compte_sender
        )
        or (
            -- or by recipient
            select
                expires_at < localtimestamp
            from
                tchatator._single_block
            where
                id_member = p_id_compte_sender
                and id_professionnal = p_id_compte_recipient
        ) then 0 -- errstatus_error
        else (select tchatator._insert_msg(p_id_compte_sender, p_id_compte_recipient, p_content))
    end
$$ language sql strict;