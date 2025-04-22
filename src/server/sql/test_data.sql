set schema 'tchatator';

create procedure use_test_data_users () as $$
insert into
    tchatator.pro (user_id, api_key, password_hash, business_name)
values
    (
        1001,
        'bb1b5a1f-a482-4858-8c6b-f4746481cffa',
        '$2y$10$cNmRElBKaejyb6ziQ3Xu/ewVP.D9/mDgYx2rqHmH8gp6RP68Qve/O', -- pro1_mdp
        'pro1 corp'
    ),
    (
        1002,
        '52d43379-8f75-4fbd-8b06-d80a87b2c2b4',
        '$2y$10$kRLzFBXkDjrC7d6HfnxLIuN9XimXNiSQwlEUgoY1i1CUui4LBEqya', -- pro2_mdp
        'pro2 inc'
    );

insert into
    tchatator.member (user_id, api_key, password_hash, user_name)
values
    (
        1003,
        '123e4567-e89b-12d3-a456-426614174000',
        '$2y$10$xuJ9YQIsn1CnN5ja76YDbeY0rX8bmtMraSxeoeMRgBr20lGDpUm/2', -- member1_mdp
        'member1'
    ),
    (
        1004,
        '9ea59c5b-bb75-4cc9-8f80-77b4ce851a0b',
        '$2y$10$WmZ.J1qFMf.m0mB3N7m6e.rYbkSjxH5yl/22ksIqjTth8Ar7jEgVO', -- member2_mdp
        'member2'
    );
$$ language sql;

create procedure use_test_data_msgs () as $$
call use_test_data_users();
insert into
    tchatator._msg (msg_id, content, user_id_sender, user_id_recipient)
values
    (1005, 'Bonjour pro1 (1er message) de la part de 5cover', 1003, 1001),
    (1006, 'Bonjour pro2 (2eme message) de la part de 5cover', 1003, 1002);
$$ language sql;