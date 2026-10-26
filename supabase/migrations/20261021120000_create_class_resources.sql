-- Links (notes, slides, videos, etc.) that people attach to a class.
-- New submissions start as unapproved and are hidden until an admin approves them.

create table if not exists class_resources (
  id bigint generated always as identity primary key,
  class_id bigint not null references classes(id) on delete cascade,
  title text not null check (char_length(title) between 1 and 120),
  url text not null check (
    char_length(url) <= 2048
    and url ~* '^https?://[^[:space:]/?#]+[^[:space:]]*$'  -- http(s) only, no spaces
  ),
  approved boolean not null default false,
  created_at timestamptz not null default now()
);

create index if not exists class_resources_class_id_idx
  on class_resources (class_id);

-- Row Level Security: the public (anon) key can only see approved links
alter table class_resources enable row level security;

drop policy if exists "Anyone can read approved resources" on class_resources;
create policy "Anyone can read approved resources"
  on class_resources for select
  using (approved = true);

grant select on public.class_resources to anon;

-- Only the server (which holds the secret key, the service_role) can add links.
-- anon gets NO insert permission, so nobody can bypass the server's checks.
grant usage on schema public to service_role;
grant select, insert on public.class_resources to service_role;
