set schema 'tchatator';

insert into
    _msg (content, id_compte_sender, id_compte_recipient)
values
    ('Bonjour MERTREM (1er message) de la part de 5cover', 3, 1),
    ('Bonjour Thiercielieux (2eme message) de la part de 5cover', 3, 2);

insert into
    professionnal (api_key, password_hash, business_name)
values
    (
        'bb1b5a1f-a482-4858-8c6b-f4746481cffa',
        '$2y$10$cNmRElBKaejyb6ziQ3Xu/ewVP.D9/mDgYx2rqHmH8gp6RP68Qve/O', -- pro1_mdp
        'pro1 corp'
    ),
    (
        '52d43379-8f75-4fbd-8b06-d80a87b2c2b4',
        '$2y$10$kRLzFBXkDjrC7d6HfnxLIuN9XimXNiSQwlEUgoY1i1CUui4LBEqya', -- pro2_mdp
        'pro2 inc'
    );

insert into
    member (api_key, password_hash, user_name)
values
    (
        '123e4567-e89b-12d3-a456-426614174000',
        '$2y$10$xuJ9YQIsn1CnN5ja76YDbeY0rX8bmtMraSxeoeMRgBr20lGDpUm/2', -- member1_mdp
        'member1'
    ),
    (
        '9ea59c5b-bb75-4cc9-8f80-77b4ce851a0b',
        '$2y$10$WmZ.J1qFMf.m0mB3N7m6e.rYbkSjxH5yl/22ksIqjTth8Ar7jEgVO', -- member2_mdp
        'member2'
    );