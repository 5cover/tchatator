set schema 'tchatator';

set
    plpgsql.extra_errors to 'all';

-- Create
create function ftg_pro_insert () returns trigger as $$
begin
    new = _insert_user(new);
    insert into _pro (
        user_id,
        business_name
    ) values (
        new.user_id,
        new.business_name
    );
    return new;
end
$$ language plpgsql;

create trigger tg_pro_insert instead of insert on pro for each row
execute function ftg_pro_insert ();

-- Update
create function ftg_pro_update () returns trigger as $$
begin
    call update_user(old, new);
    update _pro
    set
        business_name = new.business_name
    where
        user_id = new.user_id;
    return new;
end
$$ language plpgsql;

create trigger tg_pro_update instead of
update on pro for each row
execute function ftg_pro_update ();

-- Delete
create function ftg_pro_delete () returns trigger as $$
begin
    delete from _user where user_id = old.id;
    return old;
end
$$ language plpgsql;

create trigger tg_pro_delete instead of delete on pro for each row
execute function ftg_pro_delete ();