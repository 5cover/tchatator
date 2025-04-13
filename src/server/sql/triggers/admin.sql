set schema 'tchatator';

set
    plpgsql.extra_errors to 'all';

-- Create
create function ftg_admin_insert () returns trigger as $$
begin
    new = _insert_user(new);
    insert into _admin (
        user_id
    ) values (
        new.user_id
    );
    return new;
end
$$ language plpgsql;

create trigger tg_admin_insert instead of insert on admin for each row
execute function ftg_admin_insert ();

-- Update
create function ftg_admin_update () returns trigger as $$
begin
    call update_user(old, new);
    return new;
end
$$ language plpgsql;

create trigger tg_admin_update instead of
update on admin for each row
execute function ftg_admin_update ();

-- Delete
create function ftg_admin_delete () returns trigger as $$
begin
    delete from _user where user_id = old.id;
    return old;
end
$$ language plpgsql;

create trigger tg_admin_delete instead of delete on admin for each row
execute function ftg_admin_delete ();