insert into storage.buckets (id, name, public)
values ('course-files', 'course-files', false)
on conflict (id) do nothing;

drop policy if exists "Authenticated users can upload course files" on storage.objects;
create policy "Authenticated users can upload course files"
  on storage.objects for insert
  to authenticated
  with check (
    bucket_id = 'course-files'
    and (storage.foldername(name))[1] ~ '^[0-9]+$'
  );
