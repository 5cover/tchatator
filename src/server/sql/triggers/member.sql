set schema 'tchatator';

set
    plpgsql.extra_errors to 'all';

-- Create
create function ftg_member_insert () returns trigger as $$
begin
    new = _insert_user(new);
    insert into _member (
        user_id,
        user_name
    ) values (
        new.user_id,
        new.user_name
    );
    return new;
end
$$ language plpgsql;

create trigger tg_member_insert instead of insert on member for each row
execute function ftg_member_insert ();

-- Update
create function ftg_member_update () returns trigger as $$
begin
    call update_user(old, new);
    update _member
    set
        user_name = new.user_name
    where
        user_id = new.user_id;
    return new;
end
$$ language plpgsql;

create trigger tg_member_update instead of
update on member for each row
execute function ftg_member_update ();

-- Delete
create function ftg_member_delete () returns trigger as $$
begin
    delete from _user where user_id = old.id;
    return old;
end
$$ language plpgsql;

create trigger tg_member_delete instead of delete on member for each row
execute function ftg_member_delete ();