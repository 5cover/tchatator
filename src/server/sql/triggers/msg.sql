set schema 'tchatator';

set
    plpgsql.extra_errors to 'all';

-- Update
create function ftg_msg_update () returns trigger as $$
begin
    if old.msg_id <> new.msg_id then
        raise 'Cannot update user_id';
    end if;
    if old.sent_at <> new.sent_at then
        raise 'Cannot update sent_at';
    end if;
    if old.user_id_sender <> new.user_id_sender then
        raise 'Cannot update user_id_sender';
    end if;
    if old.user_id_recipient <> new.user_id_recipient then
        raise 'Cannot update user_id_recipient';
    end if;

    update tchatator._msg
    set
        edited_age = tchatator._seconds_diff(localtimestamp, sent_at),
        content = new.content,
        read_age = new.read_age,
        edited_age = new.edited_age
    where
        msg_id = old.msg_id;
    return new;
end
$$ language plpgsql;

create trigger tg_msg_update
instead of
update on msg for each row
execute function ftg_msg_update ();

-- Delete
create function ftg_msg_delete () returns trigger as $$
begin
    assert
        deleted_age is null from tchatator._msg where msg_id = old.msg_id,
        'Attempted to delete an already deleted message';
    update tchatator._msg
    set
        deleted_age = tchatator._seconds_diff(localtimestamp, sent_at)
    where
        msg_id = old.msg_id;
    return old;
end
$$ language plpgsql;

create trigger tg_msg_delete instead of delete on msg for each row
execute function ftg_msg_delete ();