set schema 'tchatator';

set
    plpgsql.extra_errors to 'all';

-- Functions stating with _ are internal
create function _insert_msg (p_user_id_sender int, p_user_id_recipient int, p_content varchar) returns int as $$
with msg_id as (insert into
    tchatator._msg (user_id_sender, user_id_recipient, content)
values
    -- consider 0 as the root user id.
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
                tchatator._member
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
                and user_id_pro = p_user_id_recipient
        ) then 0 -- errstatus_error
        else (select tchatator._insert_msg(p_user_id_sender, p_user_id_recipient, p_content))
    end
$$ language sql strict;

create function _insert_user (inout new record) as $$
begin
    insert into
        tchatator._user (user_id, api_key, password_hash)
    values
        (new.user_id, new.api_key, new.password_hash)
    returning
        user_id into new.user_id;
end
$$ language plpgsql strict;

create procedure _update_user (old record, new record) as $$
begin
    if old.user_id <> new.user_id then
        raise 'Cannot update user_id';
    end if;

    update tchatator._user
    set
        api_key = new.api_key,
        password_hash = new.password_hash
    where
        user_id = new.user_id;
end
$$ language plpgsql;

-- Computes the difference in seconds between two timestamps
create function _seconds_diff (t1 timestamp, t2 timestamp) returns int as $$
select extract(epoch from t1 - t2)::int;
$$ language sql strict immutable;