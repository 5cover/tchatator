set schema 'tchatator';

set
    plpgsql.extra_errors to 'all';

create function _insert_msg (p_user_id_sender int, p_user_id_recipient int, p_content varchar) returns int as $$
with msg_id as (insert into
    tchatator._msg (user_id_sender, user_id_recipient, content)
values
    -- condisder 0 as the admin user id.
    (nullif(p_user_id_sender, 0), p_user_id_recipient, p_content)
returning
    msg_id
) table msg_id
$$ language sql strict;

create function send_msg (p_user_id_sender int, p_user_id_recipient int, p_content varchar) returns int as $$
select
    case
        when (
            -- check is not blocked globally
            select
                full_block_expires_at < localtimestamp
            from
                _member
            where
                user_id = p_user_id_sender
        )
        or (
            -- or by recipient
            select
                expires_at < localtimestamp
            from
                tchatator._single_block
            where
                user_id_member = p_user_id_sender
                and user_id_professionnal = p_user_id_recipient
        ) then 0 -- errstatus_error
        else (select tchatator._insert_msg(p_user_id_sender, p_user_id_recipient, p_content))
    end
$$ language sql strict;